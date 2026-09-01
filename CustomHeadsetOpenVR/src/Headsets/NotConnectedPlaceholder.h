#pragma once

#include "openvr_driver.h"

/**
 * A placeholder tracked device that represents a headset that is not connected.
 * It can be registered with SteamVR so that the headset shows up as a
 * disconnected wireless HMD instead of no device being present at all.
 */
class NotConnectedPlaceholder : public vr::ITrackedDeviceServerDriver{
public:
	// returns VRInitError_Driver_WirelessHmdNotConnected as this device is never connected
	vr::EVRInitError Activate(uint32_t unObjectId) override;
	// no cleanup is needed for this placeholder
	void Deactivate() override;
	// no standby behaviour for this placeholder
	void EnterStandby() override;
	// this placeholder exposes no components
	void* GetComponent(const char* pchComponentNameAndVersion) override;
	// no debug requests are handled by this placeholder
	void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
	// this interface is unused and will never be called
	vr::DriverPose_t GetPose() override;
};
