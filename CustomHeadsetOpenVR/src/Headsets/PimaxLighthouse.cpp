#include "PimaxLighthouse.h"

#include "../Helpers/EyeTrackingOutput.h"

bool PimaxLighthouseShim::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	std::string trackingSystem = vr::VRProperties()->GetStringProperty(container, vr::Prop_TrackingSystemName_String);
	std::string manufacturer = vr::VRProperties()->GetStringProperty(container, vr::Prop_ManufacturerName_String);
	if(GetInfo().connected && (model == "Pimax Dream Air" || model == "REF-HMD") && manufacturer == "Pimax" && trackingSystem == "lighthouse"){
		return true;
	}
	return false;
}

Config::BaseHeadsetConfig& PimaxLighthouseShim::GetConfig(){
	return GetHeadsetConfig();
}

Config::BaseHeadsetConfig& PimaxLighthouseShim::GetConfigOld(){
	return GetHeadsetConfigOld();
}

void PimaxLighthouseShim::PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue){
	// We need to set this config value before UpdateSettings() runs.
	// This is only necessary when using the PimaxDistortionProfile.
	pvr_setIntConfig(GetPvrSession(), "view_rotation_fix", GetConfig().parallelProjection);

	if (HasEyeTracking()) {
		eyeTrackingOutput.Initialize();
	}

	returnValue = vr::VRInitError_None;
	BaseHeadsetShim::PosTrackedDeviceActivate(unObjectId, returnValue);
}

void PimaxLighthouseShim::SubDeactivate(){
	StopEyeTracking();
}

void PimaxLighthouseShim::RunFrame(){
	// We need to set this config value before UpdateSettings() runs.
	// This is only necessary when using the PimaxDistortionProfile.
	pvr_setIntConfig(GetPvrSession(), "view_rotation_fix", GetConfig().parallelProjection);

	BaseHeadsetShim::RunFrame();

	// Make sure to run BaseHeadsetShim::RunFrame() for housekeeping before checking for lost connection.
	if (CheckDeviceLost()) {
		return;
	}

	if(HasEyeTracking() && GetConfig().enableEyeTracking){
		StartEyeTracking();
	} else {
		StopEyeTracking();
	}
	eyeTrackingOutput.ipd = (float)(GetConfig().ipd + GetConfig().ipdOffset);
	eyeTrackingOutput.RunFrame();
}

void PimaxLighthouseShim::HandleEvent(const vr::VREvent_t& event){
	switch (event.eventType) {
	case vr::VREvent_SceneApplicationChanged:
		// Signal Pimax Play to perform a MagicAttach (DFR injector) when a new scene app started.
		pvr_setIntConfig(GetPvrSession(), "openvr_client_changed", event.data.process.pid);
		break;
	}
}


extern "C" {
// cant be bothered, implement them here
void* zcalloc(void* opaque, unsigned int items, unsigned int size){
	return malloc(items * size);
}
void zcfree(void* opaque, void* address){
	free(address);
}
const char * z_errmsg[12]{"","","","","","","","","","","",""};
}