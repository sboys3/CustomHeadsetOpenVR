#include "FakeHeadset.h"


bool FakeHeadset::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container){
	// This is not overriding anything, this is the headset
	return true;
}

Config::BaseHeadsetConfig& FakeHeadset::GetConfig(){
	return driverConfig.fakeHeadset;
}

Config::BaseHeadsetConfig& FakeHeadset::GetConfigOld(){
	return driverConfigOld.fakeHeadset;
}

void FakeHeadset::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){
	// call parent
	BaseHeadsetShim::PosTrackedDeviceActivate(unObjectId, returnValue);
	
	// get property container
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	
	// set name
	vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, "Fake Custom Headset");
	// The distance from the user's eyes to the display in meters. This is used for reprojection.
	vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserHeadToEyeDepthMeters_Float, 0.02f);
	// set default fps
	vr::VRProperties()->SetFloatProperty(container, vr::Prop_DisplayFrequency_Float, 0.0f);
}

void FakeHeadset::RunFrame(){
	// call parent
	BaseHeadsetShim::RunFrame();
	
	if(!isActive){
		// SteamVR has not chosen this device
		return;
	}
	
	vr::DriverPose_t pose = {0};

	// These need to be set to be valid quaternions. The device won't appear otherwise.
	pose.qWorldFromDriverRotation.w = 1.f;
	pose.qDriverFromHeadRotation.w = 1.f;

	// Slowly rotate around the Y-axis (vertical axis)
	double angle = lastFrameTime * 0.01f;
	float cosHalfAngle = (float)cos(angle);
	float sinHalfAngle = (float)sin(angle);
	
	pose.qRotation.x = 0.f;
	pose.qRotation.y = sinHalfAngle; // Y-axis rotation
	pose.qRotation.z = 0.f;
	pose.qRotation.w = cosHalfAngle;

	pose.vecPosition[0] = 0.0f;
	pose.vecPosition[1] = (float)(sin(lastFrameTime * 0.5) * 0.1 + 1.0); // slowly move the hmd up and down.
	pose.vecPosition[2] = 0.0f;

	// The pose we provided is valid.
	// This should be set is
	pose.poseIsValid = true;

	// Our device is always connected.
	// In reality with physical devices, when they get disconnected,
	// set this to false and icons in SteamVR will be updated to show the device is disconnected
	pose.deviceIsConnected = true;

	// The state of our tracking. For our virtual device, it's always going to be ok,
	// but this can get set differently to inform the runtime about the state of the device's tracking
	// and update the icons to inform the user accordingly.
	pose.result = vr::TrackingResult_Running_OK;

	// For HMDs we want to apply rotation/motion prediction
	pose.shouldApplyHeadModel = true;
	
	vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(vr::DriverPose_t));
}
