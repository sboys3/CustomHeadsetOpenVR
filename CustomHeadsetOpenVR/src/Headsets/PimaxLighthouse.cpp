#include "PimaxLighthouse.h"

#include "../Helpers/EyeTrackingOutput.h"

bool PimaxLighthouseShim::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	std::string trackingSystem = vr::VRProperties()->GetStringProperty(container, vr::Prop_TrackingSystemName_String);
	std::string manufacturer = vr::VRProperties()->GetStringProperty(container, vr::Prop_ManufacturerName_String);
	if(GetInfo().connected && (model.find("Pimax") != std::string::npos || (model == "REF-HMD" && manufacturer.find("Pimax") != std::string::npos)) && trackingSystem == "lighthouse"){
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

	// Update the battery level (Crystal OG).
	const vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);
	const int batteryPercentage = pvr_getTrackedDeviceIntProperty(
		GetPvrSession(), pvrTrackedDevice_HMD, pvrTrackedDeviceProp_BatteryPercent_int, -1);
	if (batteryPercentage > 0) {
		vr::VRProperties()->SetFloatProperty(container, vr::Prop_DeviceBatteryPercentage_Float, batteryPercentage / 100.f);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
	}

	PollMagicAttach();
}

void PimaxLighthouseShim::HandleEvent(const vr::VREvent_t& event){
	switch (event.eventType) {
	case vr::VREvent_SceneApplicationChanged:
		SetSceneApplicationProcess(event.data.process.pid);
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