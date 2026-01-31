
#include "GenericHeadset.h"
#include "../Config/ConfigLoader.h"

#include "../../../ThirdParty/easywsclient/easywsclient.hpp"

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
	
	if(driverConfig.generalHeadset.useViveBluetooth){
		uint64_t existingFeatures = vr::VRProperties()->GetUint64Property(container, vr::Prop_AdditionalRadioFeatures_Uint64);
		vr::VRProperties()->SetUint64Property(container, vr::Prop_AdditionalRadioFeatures_Uint64, existingFeatures | vr::AdditionalRadioFeatures_HTCLinkBox);
	}
	
	// currently this does nothing else so disable the shim
	shimActive = false;
}

void GenericHeadsetShim::HandleEvent(const vr::VREvent_t &event){
	if(driverConfig.takeCompositorScreenshots && event.eventType == vr::VREvent_ScreenshotTriggered){
		DriverLog("Screenshot triggered, sending screenshot request to vrcompositor");
		easywsclient::WebSocket* websocket = easywsclient::WebSocket::from_url("ws://127.0.0.1:27062/", "http://127.0.0.1:27062");
		if(websocket == nullptr){
			DriverLog("Failed to create websocket");
			return;
		}
		websocket->send("mailbox_send vrcompositor_mailbox {\"type\":\"screenshot_request\"}");
		websocket->poll();
		websocket->close();
		websocket->poll(); // final poll to close the connection
		delete websocket;
	}
}