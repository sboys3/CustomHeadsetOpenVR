#include "NotConnectedPlaceholder.h"

vr::EVRInitError NotConnectedPlaceholder::Activate(uint32_t unObjectId){
	// this device never connects. returning this error makes SteamVR treat the
	// device as a wireless HMD that is not connected
	return vr::VRInitError_Driver_WirelessHmdNotConnected;
}

void NotConnectedPlaceholder::Deactivate(){}

void NotConnectedPlaceholder::EnterStandby(){}

void* NotConnectedPlaceholder::GetComponent(const char* pchComponentNameAndVersion){
	// this placeholder exposes no components
	return nullptr;
}

void NotConnectedPlaceholder::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize){
	// no debug requests are handled
}

vr::DriverPose_t NotConnectedPlaceholder::GetPose(){
	// this interface is unused and will never be called
	return {0};
}
