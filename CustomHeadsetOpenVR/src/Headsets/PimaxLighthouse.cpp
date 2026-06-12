#include "PimaxLighthouse.h"

bool PimaxLighthouseShim::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	std::string trackingSystem = vr::VRProperties()->GetStringProperty(container, vr::Prop_TrackingSystemName_String);
	if(GetInfo().connected && model == "Pimax Dream Air" && trackingSystem == "lighthouse"){
		return true;
	}
	return false;
}

Config::BaseHeadsetConfig& PimaxLighthouseShim::GetConfig(){
	return driverConfig.dreamAir;
}

Config::BaseHeadsetConfig& PimaxLighthouseShim::GetConfigOld(){
	return driverConfigOld.dreamAir;
}


void PimaxLighthouseShim::SubActivate(vr::PropertyContainerHandle_t container){
	eyeTracking.eyeRotation = defaultDriverConfig.dreamAir.eyeRotation;
	eyeTracking.enabled = HasEyeTracking() && GetConfig().enableEyeTracking;
	eyeTracking.Initialize();
}

void PimaxLighthouseShim::SubDeactivate(){
	eyeTracking.Shutdown();
}

void PimaxLighthouseShim::SubRunFrame(){
	eyeTracking.ipd = GetConfig().ipd + GetConfig().ipdOffset;
	eyeTracking.enabled = HasEyeTracking() && GetConfig().enableEyeTracking;
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