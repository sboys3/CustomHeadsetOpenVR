#include "DeviceProvider.h"
#include "DriverLog.h"
#include "DeviceShim.h"
#include "CompositorPlugin.h"

#include "Hooking/InterfaceHookInjector.h"

#include "../Headsets/MeganeX8K.h"
#include "../Headsets/GenericHeadset.h"

#include "../Config/ConfigLoader.h"


// general driver functions
vr::EVRInitError CustomHeadsetDeviceProvider::Init(vr::IVRDriverContext *pDriverContext){
	// initialise this driver
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	char driverPath[2048];
	vr::VRResources()->GetResourceFullPath("", "", driverPath, sizeof(driverPath));
	driverConfigLoader.info.steamvrResources = driverPath;
	vr::VRResources()->GetResourceFullPath("{CustomHeadsetOpenVR}", "", driverPath, sizeof(driverPath));
	driverConfigLoader.info.driverResources = driverPath;
	driverConfigLoader.Start();
	// inject hooks into functions
	InjectHooks(this, pDriverContext);
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
	// acquire driverConfig.configLock for the duration of this function
	std::lock_guard<std::mutex> lock(driverConfigLock);
	
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
				DriverLog("Received context collection event for device with ID: %d, Context: %p", id, ctx);	
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
		
		if(vrevent.eventType == vr::VREvent_ProcessConnected && customShaderEnabled){
			// check new processes and inject if they are the compositor
			InjectCompositorPlugin(vrevent.data.process.pid);
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
	// clear update flag at end of frame
	driverConfig.hasBeenUpdated = false;
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
	if(driverConfig.forceTracking){
		pose.poseIsValid = true;
		if(pose.result != vr::TrackingResult_Fallback_RotationOnly){
			pose.result = vr::TrackingResult_Running_OK;
		}
	}
	return true;
}

bool CustomHeadsetDeviceProvider::HandleDeviceAdded(const char *&pchDeviceSerialNumber, vr::ETrackedDeviceClass &eDeviceClass, vr::ITrackedDeviceServerDriver *&pDriver){
	DriverLog("HandleDeviceAdded %s\n", pchDeviceSerialNumber);
	if(eDeviceClass == vr::TrackedDeviceClass_HMD){
		
		// add more shims here, they can stack and none of the functions are particularly hot
		// later shims can override earlier shims
		// the PosTrackedDeviceActivate function will likely have enough information that you can decide if it is the device you want and can then set shimActive to false to deactivate the shim
		
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
