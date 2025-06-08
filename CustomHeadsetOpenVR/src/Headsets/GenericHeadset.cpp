
#include "GenericHeadset.h"
#include "../Config/ConfigLoader.h"

void GenericHeadsetShim::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){
	if(returnValue == vr::VRInitError_None && driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::None){
		// mark that there has been some headset found
		driverConfigLoader.info.connectedHeadset = ConfigLoader::HeadsetType::Other;
	}
	driverConfigLoader.WriteInfo();
}