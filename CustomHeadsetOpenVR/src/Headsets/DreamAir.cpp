#include "DreamAir.h"


bool DreamAirShim::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	std::string trackingSystem = vr::VRProperties()->GetStringProperty(container, vr::Prop_TrackingSystemName_String);
	if(model == "Pimax Dream Air" && trackingSystem == "lighthouse"){
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



extern "C" {
// cant be bothered, implement them here
void* zcalloc(void* opaque, unsigned int items, unsigned int size){
	return malloc(items * size);
}
void zcfree(void* opaque, void* address){
	free(address);
}
const char * z_errmsg[10]{"","","","","","","","","",""};
}