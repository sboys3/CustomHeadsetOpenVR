#include "MeganeX8K.h"


bool MeganeX8KShim::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	if(model == "MeganeX 8K Mark II"){
		headsetRevision = 2;
	}
	if(model == "MeganeX superlight 8K" || model == "MeganeX 8K Mark II"){
		return true;
	}
	return false;
}

Config::BaseHeadsetConfig& MeganeX8KShim::GetConfig(){
	return driverConfig.meganeX8K;
}

Config::BaseHeadsetConfig& MeganeX8KShim::GetConfigOld(){
	return driverConfigOld.meganeX8K;
}