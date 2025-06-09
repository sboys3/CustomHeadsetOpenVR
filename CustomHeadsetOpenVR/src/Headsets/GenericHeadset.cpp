
#include "GenericHeadset.h"
#include "../Config/ConfigLoader.h"

void GenericHeadsetShim::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	
	std::string modelNumber = vr::VRProperties()->GetStringProperty(container, vr::Prop_ModelNumber_String);
	if(modelNumber == "Vive. MV"){
		driverConfigLoader.info.connectedHeadset = ConfigLoader::HeadsetType::Vive;
	}
	if(returnValue == vr::VRInitError_None && driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::None){
		// mark that there has been some headset found
		driverConfigLoader.info.connectedHeadset = ConfigLoader::HeadsetType::Other;
	}
	driverConfigLoader.WriteInfo();
}