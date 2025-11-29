#pragma once

#include <set>
#include <map>
#include <vector>

#include "openvr_driver.h"

class ShimDefinition;

#define VREvent_VendorSpecific_ContextCollection (vr::EVREventType)(vr::VREvent_VendorSpecific_Reserved_Start + 5872)
#define VREvent_VendorSpecific_ContextCollection_MagicDataNumber 32643216579172981


class CustomHeadsetDeviceProvider : public vr::IServerTrackedDeviceProvider
{
public:
	vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext) override;
	const char *const *GetInterfaceVersions() override;
	
	// called by the main loop of the server
	void RunFrame() override;
	// deprecated function, but still must be defined
	bool ShouldBlockStandbyMode() override;
	// SteamVR is entering/leaving standby mode
	void EnterStandby() override;
	void LeaveStandby() override;
	// cleanup on exit
	void Cleanup() override;
	
	// handle hook of TrackedDevicePoseUpdated
	bool HandleDevicePoseUpdated(uint32_t openVRID, vr::DriverPose_t &pose);
	// handle hook of TrackedDeviceAdded
	bool HandleDeviceAdded(const char* &pchDeviceSerialNumber, vr::ETrackedDeviceClass &eDeviceClass, vr::ITrackedDeviceServerDriver* &pDriver);
	// set of driver conexts collected by the hooking process
	std::set<vr::IVRDriverContext*> driverContexts = {};
	// map of driver contexts by device id
	// this is populated by VREvent_VendorSpecific_ContextCollection events
	std::map<uint32_t, vr::IVRDriverContext*> driverContextsByDeviceId = {};
	// sends out VREvent_VendorSpecific_ContextCollection events for a given device id
	// after some time, the driverContextsByDeviceId map should be contain the context for this device
	void SendContextCollectionEvents(uint32_t id);
	// attempt to send the event if the context is available, returns true if successful.
	// if false was returned the message has queued to be sent if the driver context can be found
	// events must be sent from the context that owns the device, so this is necessary
	bool SendVendorEvent(uint32_t unWhichDevice, vr::EVREventType eventType, const vr::VREvent_Data_t & eventData, double eventTimeOffset);
	// a set of all shim objects to manage
	// this allows them to have RunThread called
	std::set<ShimDefinition*> shims;
private:
	struct QueuedEvent {
		vr::EVREventType eventType;
		vr::VREvent_Data_t eventData;
		double eventTimeOffset;
	};
	// events that are waiting for a context to be found
	std::map<uint32_t, std::vector<QueuedEvent>> queuedEvents = {};
	bool customShaderEnabled = false;
};