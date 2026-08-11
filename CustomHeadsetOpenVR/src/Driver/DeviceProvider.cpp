#include "DeviceProvider.h"
#include "DriverLog.h"
#include "DeviceShim.h"
#include "CompositorPlugin.h"
#include "HidModifier.h"
#include "DriverLockout.h"

#include <chrono>

#include "Hooking/InterfaceHookInjector.h"

#include "../Headsets/MeganeX8K.h"
#include "../Headsets/GenericHeadset.h"
#include "../Headsets/FakeHeadset.h"
#include "../Headsets/PimaxLighthouse.h"
#ifdef PVR_EXISTS
#include "../Headsets/PimaxSlam.h"
#endif
#include "../Helpers/EyeTrackingOutput.h"
#include "../Helpers/AAPVRBlocker.h"
#include "../Helpers/MiscHelper.h"

#include "../Config/ConfigLoader.h"

bool lockedOut = false;

bool CustomHeadsetDeviceProvider::hasHeadsetConnected = false;

// general driver functions
vr::EVRInitError CustomHeadsetDeviceProvider::Init(vr::IVRDriverContext *pDriverContext){
	// initialise this driver
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	
	vr::DriverHandle_t driverHandle = vr::VRDriverHandle();

	std::string driverName;
	uint32_t count = vr::VRDriverManager()->GetDriverCount();
	for (uint32_t i = 0; i < count; i++) {
		char name[128];
		vr::VRDriverManager()->GetDriverName(i, name, sizeof(name));
		if (vr::VRDriverManager()->GetDriverHandle(name) == driverHandle) {
			driverName = name;
			break;
		}
	}
	driverConfigLoader.info.driverName = driverName;
	char driverPath[2048];
	vr::VRResources()->GetResourceFullPath("", "", driverPath, sizeof(driverPath));
	driverConfigLoader.info.steamvrResources = driverPath;
	vr::VRResources()->GetResourceFullPath(("{" + driverName + "}").c_str(), "", driverPath, sizeof(driverPath));
	driverConfigLoader.info.driverResources = driverPath;
	
	DriverLog("Initializing %s", driverName.c_str());
	
	// Driver lockout: When this is a vendor-specific driver (not vendor-neutral),
	// check if the vendor-neutral driver (CustomHeadsetOpenVR) is enabled.
	// If the neutral driver is enabled, this vendor driver is locked out.
	#ifndef VENDOR_NEUTRAL
	DriverLog("Running in vendor-specific driver mode");
	if(IsNeutralDriverEnabled()){
		DriverLog("Vendor-specific driver locked out because the vendor-neutral driver (CustomHeadsetOpenVR) is enabled.");
		lockedOut = true;
		driverConfigLoader.CreateDirectories();
		driverConfigLoader.WriteInfo();
		return vr::VRInitError_None;
	}
	#endif
	
	// Write a setting so that the section of thes driver is always defined in the settings file for other drivers to detect
	WriteHasBeenRunSetting(driverName.c_str());
	
	driverConfigLoader.Start();
	// inject hooks into functions
	InjectHooks(this, pDriverContext);
	hidModifier.InjectHooks();
	if(AAPVRShouldBlock()){
		AAPVRLighthouseUnblockerInjectHooks();
	}
	

	// the shim classes can be used to implement entirely new headsets, not just shim existing ones
	if(driverConfig.fakeHeadset.enable){
		FakeHeadset* fakeHeadsetImplementation = new FakeHeadset();
		fakeHeadsetImplementation->deviceProvider = this;
		shims.insert(fakeHeadsetImplementation);
		vr::ITrackedDeviceServerDriver* driver = new ShimTrackedDeviceDriver(fakeHeadsetImplementation, nullptr);
		vr::VRServerDriverHost()->TrackedDeviceAdded("FakeCustomHMD", vr::TrackedDeviceClass_HMD, driver);
	}
	
	return vr::VRInitError_None;
}
const char *const *CustomHeadsetDeviceProvider::GetInterfaceVersions(){
	return vr::k_InterfaceVersions;
}
bool CustomHeadsetDeviceProvider::ShouldBlockStandbyMode(){
	return false;
}
void CustomHeadsetDeviceProvider::Cleanup(){}
void CustomHeadsetDeviceProvider::EnterStandby(){}
void CustomHeadsetDeviceProvider::LeaveStandby(){}

void DebugEventLog(const vr::VREvent_t& vrevent){
	DriverLog("Event type: %d", vrevent.eventType);
	switch(vrevent.eventType){
		case vr::VREvent_PropertyChanged:
			DriverLog("Property changed: %i", vrevent.data.property.prop);
			break;
		case vr::VREvent_Compositor_DisplayReconnected:
			DriverLog("Compositor display reconnected");
			break;
		case vr::VREvent_ProcessConnected:
			DriverLog("Process connected %i", vrevent.data.process.pid);
			break;
	}
}

void CustomHeadsetDeviceProvider::RunFrame(){
	
	// Static variables for benchmark tracking
	static auto startupTime = std::chrono::high_resolution_clock::now();
	static double maxDurationMs = 0.0;
	static double totalDurationMs = 0.0;
	static int frameCount = 0;
	static bool resetAt60 = false;
	static bool resetAt600 = false;
	
	if(lockedOut){
		return;
	}
	
	// Benchmark timing - record start
	auto frameStart = std::chrono::high_resolution_clock::now();
	
	double now = std::chrono::duration<double>(frameStart.time_since_epoch()).count();
	
	// acquire driverConfig.configLock for the duration of this function
	std::lock_guard<std::mutex> lock(driverConfigLock);
	
	hidModifier.RunFrame();
	
	#ifdef HAS_PRIVATE
	if(driverConfig.onlyHandlePrivateFunctionality){
		driverConfig.hasBeenUpdated = false;
		return;
	}
	#endif
		
	// process events that were submitted for this frame.
	vr::VREvent_t vrevent{};
	while(vr::VRServerDriverHost()->PollNextEvent(&vrevent, sizeof(vr::VREvent_t))){
		// DebugEventLog(vrevent);
		if(vrevent.eventType == VREvent_VendorSpecific_ContextCollection){
			// receive and store data from successful context collection events
			vr::VREvent_Reserved_t data = vrevent.data.reserved;
			if(data.reserved0 == VREvent_VendorSpecific_ContextCollection_MagicDataNumber){
				// add context based on the event data.
				uint32_t id = static_cast<uint32_t>(data.reserved1);
				vr::IVRDriverContext* ctx = (vr::IVRDriverContext*)data.reserved2;
				// logging here seems to deadlock on occasion
				// DriverLog("Received context collection event for device with ID: %d, Context: %p", id, ctx);
				driverContextsByDeviceId[id] = ctx;
				// send any queued events
				if(queuedEvents.find(id) != queuedEvents.end()){
					for(const auto& event : queuedEvents[id]){
						SendVendorEvent(id, event.eventType, event.eventData, event.eventTimeOffset);
					}
					queuedEvents.erase(id);
				}
			}
		}
		if(vrevent.eventType == vr::VREvent_TrackedDeviceActivated){
			if(vrevent.trackedDeviceIndex == 0){
				hasHeadsetConnected = true;
			}
			// set nonNativeHeadsetFound if a device with a direct mode component is found
			vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(vrevent.trackedDeviceIndex);
			if(container){
				if(vr::VRProperties()->GetBoolProperty(container, vr::Prop_HasDriverDirectModeComponent_Bool)){
					DriverLog("Device %d has driver direct mode component: %s", vrevent.trackedDeviceIndex, vr::VRProperties()->GetBoolProperty(container, vr::Prop_HasDriverDirectModeComponent_Bool) ? "true" : "false");
					driverConfigLoader.info.nonNativeHeadsetFound = true;
					driverConfigLoader.WriteInfo();
				}
			}
		}
		if(vrevent.eventType == vr::VREvent_DashboardActivated){
			if(!driverConfigLoader.info.isDashboardOpen){
				driverConfigLoader.info.isDashboardOpen = true;
				driverConfigLoader.WriteInfo();
			}
		}
		if(vrevent.eventType == vr::VREvent_DashboardDeactivated){
			if(driverConfigLoader.info.isDashboardOpen){
				driverConfigLoader.info.isDashboardOpen = false;
				driverConfigLoader.WriteInfo();
			}
		}
		if(vrevent.eventType == vr::VREvent_ProcessConnected && customShaderEnabled){
			// check new processes and inject if they are the compositor
			InjectCompositorPlugin(vrevent.data.process.pid);
		}
		for(auto shim : shims){
			shim->HandleEvent(vrevent);
		}
	}
	for(auto shim : shims){
		if(shim->shimActive){
			shim->RunFrame();
		}
	}
	if(!customShaderEnabled && IsCustomShaderEnabled()){
		// try to inject when it is first enabled
		InjectCompositorPlugin();
		customShaderEnabled = true;
	}
	eyeTrackingOutput.RunFrame();
	
	// if no headset is connected see if any can be provided every 4 seconds
	if(!hasHeadsetConnected && lastHeadsetProvideTime + 4 < now){
		lastHeadsetProvideTime = now;
		// For Pimax SLAM headsets, load our own driver instead of shimming another driver.
		#ifdef PVR_EXISTS
		if(PimaxCommon::IsSlamHeadsetConnected()) {
			PimaxSlamDriver* pimaxSlamImplementation = new PimaxSlamDriver();
			pimaxSlamImplementation->deviceProvider = this;
			shims.insert(pimaxSlamImplementation);
			vr::ITrackedDeviceServerDriver* driver = new ShimTrackedDeviceDriver(pimaxSlamImplementation, nullptr);
			vr::VRServerDriverHost()->TrackedDeviceAdded("PimaxSlamCustomHMD", vr::TrackedDeviceClass_HMD, driver);
		}
		#endif // PVR_EXISTS
	}
	
	// clear update flag at end of frame
	driverConfig.hasBeenUpdated = false;
	
	// Benchmark timing - calculate elapsed time
	auto frameEnd = std::chrono::high_resolution_clock::now();
	double elapsedMs = std::chrono::duration<double, std::micro>(frameEnd - frameStart).count() / 1000.0;
	
	// Accumulate for average
	totalDurationMs += elapsedMs;
	frameCount++;
	double avgDurationMs = totalDurationMs / frameCount;
	
	// Check for new maximum and log
	if(elapsedMs > maxDurationMs){
		maxDurationMs = elapsedMs;
		double secondsSinceStartup = std::chrono::duration<double>(frameStart - startupTime).count();
		DriverLog("RunFrame new max: %.3f ms (avg: %.3f ms, %d frames, %.1f s since startup)", elapsedMs, avgDurationMs, frameCount, secondsSinceStartup);
	}
	
	// Check for reset points at 60s and 600s since startup
	double secondsSinceStartupEarly = std::chrono::duration<double>(frameStart - startupTime).count();
	if(!resetAt60 && secondsSinceStartupEarly >= 60.0){
		maxDurationMs = 0.0;
		totalDurationMs = 0.0;
		frameCount = 0;
		resetAt60 = true;
		DriverLog("RunFrame max and average reset at 60s since startup");
	}
	if(!resetAt600 && secondsSinceStartupEarly >= 600.0){
		maxDurationMs = 0.0;
		totalDurationMs = 0.0;
		frameCount = 0;
		resetAt600 = true;
		DriverLog("RunFrame max and average reset at 600s since startup");
	}
}

void CustomHeadsetDeviceProvider::SendContextCollectionEvents(uint32_t id){
	for(auto driverContext : driverContexts){
		vr::EVRInitError eError = vr::VRInitError_None;
		vr::IVRServerDriverHost* VRServerDriverHost =  (vr::IVRServerDriverHost *)driverContext->GetGenericInterface(vr::IVRServerDriverHost_Version, &eError);
		// store data in event
		vr::VREvent_Data_t data = {VREvent_VendorSpecific_ContextCollection_MagicDataNumber, (uint64_t)id, (uint64_t)driverContext};
		// this event will only succeed for the driver that owns the id
		VRServerDriverHost->VendorSpecificEvent(id, VREvent_VendorSpecific_ContextCollection, data, 0);
	}
}

bool CustomHeadsetDeviceProvider::SendVendorEvent(uint32_t unWhichDevice, vr::EVREventType eventType, const vr::VREvent_Data_t & eventData, double eventTimeOffset){
	if(driverContextsByDeviceId.find(unWhichDevice) != driverContextsByDeviceId.end()){
		vr::EVRInitError eError = vr::VRInitError_None;
		vr::IVRServerDriverHost* VRServerDriverHost =  (vr::IVRServerDriverHost *)driverContextsByDeviceId[unWhichDevice]->GetGenericInterface(vr::IVRServerDriverHost_Version, &eError);
		VRServerDriverHost->VendorSpecificEvent(unWhichDevice, eventType, eventData, eventTimeOffset);
		return true;
	}else{
		// try to find context and queue for later
		SendContextCollectionEvents(unWhichDevice);
		if(queuedEvents.find(unWhichDevice) == queuedEvents.end()){
			queuedEvents[unWhichDevice] = {};
		}
		queuedEvents[unWhichDevice].push_back({eventType, eventData, eventTimeOffset});
		return false;
	}
}

bool CustomHeadsetDeviceProvider::HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t &pose){
	if(driverConfig.forceTracking || (driverConfig.forceTrackingHeadsetOnly && openVRID == 0)){
		pose.poseIsValid = true;
		if(pose.result != vr::TrackingResult_Fallback_RotationOnly){
			pose.result = vr::TrackingResult_Running_OK;
		}
	}
	return true;
}

bool CustomHeadsetDeviceProvider::HandleDeviceAdded(const char *&pchDeviceSerialNumber, vr::ETrackedDeviceClass &eDeviceClass, vr::ITrackedDeviceServerDriver *&pDriver){
	#ifdef HAS_PRIVATE
	if(driverConfig.onlyHandlePrivateFunctionality){
		return true;
	}
	#endif
	DriverLog("HandleDeviceAdded %s\n", pchDeviceSerialNumber);
	
	for(std::string serial : driverConfig.serialBlacklist){
		if(pchDeviceSerialNumber == serial){
			DriverLog("Device %s is blacklisted, removing", pchDeviceSerialNumber);
			return false;
		}
	}
	
	if(hidModifier.originalDeviceClasses.find(std::string(pchDeviceSerialNumber)) != hidModifier.originalDeviceClasses.end()){
		DriverLog("Device %s restoring original class %d", pchDeviceSerialNumber, hidModifier.originalDeviceClasses[std::string(pchDeviceSerialNumber)]);
		eDeviceClass = (vr::ETrackedDeviceClass)hidModifier.originalDeviceClasses[std::string(pchDeviceSerialNumber)];
	}
	
	#ifndef FULLY_BLOCK_AAPVR
	if(AAPVRShouldBlock()){
		APPVRDeviceBlocker* appvrDeviceBlocker = new APPVRDeviceBlocker();
		shims.insert(appvrDeviceBlocker);
		pDriver = new ShimTrackedDeviceDriver(appvrDeviceBlocker, pDriver);
	}
	#endif
	
	if(eDeviceClass == vr::TrackedDeviceClass_HMD){
		
		// add more shims here, they can stack and none of the functions are particularly hot
		// later shims can override earlier shims
		// the PosTrackedDeviceActivate function will likely have enough information that you can decide if it is the device you want and can then set shimActive to false to deactivate the shim
		
		// TODO: validate the interface versions of drivers and make the shims conform to versions to prevent potential crashes
		
		
		#ifdef __linux__
		PimaxCommon::TryDirectConnection();
		if(PimaxCommon::GetInfo().directConnected){
			DriverLog("Pimax direct connection detected");
			PISTARTLINUX
		}
		#endif
		if(PimaxCommon::IsLighthouseHeadsetConnected()){
			PimaxLighthouseShim* pimaxLighthouseShim = new PimaxLighthouseShim();
			pimaxLighthouseShim->deviceProvider = this;
			shims.insert(pimaxLighthouseShim);
			pDriver = new ShimTrackedDeviceDriver(pimaxLighthouseShim, pDriver);
		}
		
		if(driverConfig.meganeX8K.enable){
			MeganeX8KShim* meganeX8KShim = new MeganeX8KShim();
			meganeX8KShim->deviceProvider = this;
			shims.insert(meganeX8KShim);
			pDriver = new ShimTrackedDeviceDriver(meganeX8KShim, pDriver); 
		}
		
		GenericHeadsetShim* genericHeadsetShim = new GenericHeadsetShim();
		genericHeadsetShim->deviceProvider = this;
		shims.insert(genericHeadsetShim);
		pDriver = new ShimTrackedDeviceDriver(genericHeadsetShim, pDriver);
	}
	// you can change eDeviceClass to change what an existing device shows up as
	
	// if false is returned the device will not be added
	return true;
}
