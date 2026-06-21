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
	Config::BaseHeadsetConfig* config = driverConfig.ConfigFromHeadsetType(GetInfo().headsetType);
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &driverConfig.dreamAir;
	}
	return PatchConfig(*config);
}

Config::BaseHeadsetConfig& PimaxLighthouseShim::GetConfigOld(){
	Config::BaseHeadsetConfig* config = driverConfigOld.ConfigFromHeadsetType(GetInfo().headsetType);
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &driverConfigOld.dreamAir;
	}
	return PatchConfig(*config);
}

void PimaxLighthouseShim::PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue){
	// We need to set this config value before UpdateSettings() runs.
	// This is only necessary when using the PimaxDistortionProfile.
	pvr_setIntConfig(GetPvrSession(), "view_rotation_fix", GetConfig().parallelProjection);

	if (HasEyeTracking()) {
		eyeTrackingOutput.Initialize();
	}

	// We need to set the mesh values before UpdateSettings() runs.
	if (GetConfig().hiddenArea.enable && GetConfig().hiddenArea.autoHiddenArea) {
		SetVisibilityMeshes();
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

	if (driverConfig.hasBeenUpdated &&
		(GetConfig().hiddenArea != GetConfigOld().hiddenArea || GetConfigOld().disableEye != GetConfig().disableEye)) {
		// We need to set the mesh values before UpdateSettings() runs.
		if (GetConfig().hiddenArea.enable && GetConfig().hiddenArea.autoHiddenArea) {
			SetVisibilityMeshes();
		}
	}

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