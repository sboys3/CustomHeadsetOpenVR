#include "DreamAir.h"


bool DreamAirShim::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	std::string trackingSystem = vr::VRProperties()->GetStringProperty(container, vr::Prop_TrackingSystemName_String);
	if(IsOpenPortEnabled() && (model == "Pimax Dream Air" || GetHmdInfo().ProductId == 0x0044) && (trackingSystem == "lighthouse" || trackingSystem == "aapvr")) {
		return true;
	}
	return false;
}

Config::BaseHeadsetConfig& DreamAirShim::GetConfig(){
	return driverConfig.dreamAir;
}

Config::BaseHeadsetConfig& DreamAirShim::GetConfigOld(){
	return driverConfigOld.dreamAir;
}


void DreamAirShim::SubActivate(vr::PropertyContainerHandle_t container){
	// TODO: Remove eye tracking bridge - aapvr driver already includes the code.
	eyeTracking.eyeRotation = defaultDriverConfig.dreamAir.eyeRotation;
	eyeTracking.enabled = GetConfig().enableEyeTracking;
	eyeTracking.Initialize();
}

void DreamAirShim::SubDeactivate(){
	eyeTracking.Shutdown();
}

void DreamAirShim::SubRunFrame(){
	eyeTracking.ipd = GetConfig().ipd + GetConfig().ipdOffset;
	eyeTracking.enabled = GetConfig().enableEyeTracking;
	// eyeTracking.eyeRotation = GetConfig().eyeRotation;
	eyeTracking.RunFrame();
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