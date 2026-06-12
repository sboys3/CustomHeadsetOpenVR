#include "PimaxSlam.h"

static constexpr uint64_t k_UniverseId = 0x50494D4158; // "PIMAX"

bool PimaxSlamDriver::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) {
	return true;
}

Config::BaseHeadsetConfig& PimaxSlamDriver::GetConfig() {
	return driverConfig.dreamAir;
}

Config::BaseHeadsetConfig& PimaxSlamDriver::GetConfigOld() {
	return driverConfigOld.dreamAir;
}

void PimaxSlamDriver::PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue) {
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

	vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "Pimax");
	vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "Pimax");
	vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, GetHmdInfo().ProductName);
	vr::VRProperties()->SetStringProperty(container, vr::Prop_RenderModelName_String, "generic_hmd");

	vr::VRProperties()->SetUint64Property(container, vr::Prop_CurrentUniverseId_Uint64, k_UniverseId);

	// TODO(mbucchia): Add shims for eye tracking

	returnValue = vr::VRInitError_None;
	BaseHeadsetShim::PosTrackedDeviceActivate(unObjectId, returnValue);
}

void PimaxSlamDriver::SubDeactivate() {
}

void PimaxSlamDriver::SubRunFrame() {
	const auto pvrNow = GetPvrTime();

	vr::DriverPose_t pose = {};
	pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
	pose.deviceIsConnected = true;
	pose.result = vr::TrackingResult_Running_OutOfRange;

	pvrTrackingState state = {};
	pvr_getTrackingState(GetPvrSession(), pvrNow, &state);
	if (state.HeadPose.StatusFlags & pvrStatus_OrientationTracked) {
		pose.vecPosition[0] = state.HeadPose.ThePose.Position.x;
		pose.vecPosition[1] = state.HeadPose.ThePose.Position.y;
		pose.vecPosition[2] = state.HeadPose.ThePose.Position.z;
		pose.qRotation.x = state.HeadPose.ThePose.Orientation.x;
		pose.qRotation.y = state.HeadPose.ThePose.Orientation.y;
		pose.qRotation.z = state.HeadPose.ThePose.Orientation.z;
		pose.qRotation.w = state.HeadPose.ThePose.Orientation.w;

		pose.vecVelocity[0] = state.HeadPose.LinearVelocity.x;
		pose.vecVelocity[1] = state.HeadPose.LinearVelocity.y;
		pose.vecVelocity[2] = state.HeadPose.LinearVelocity.z;

		pose.vecAngularVelocity[0] = state.HeadPose.AngularVelocity.x;
		pose.vecAngularVelocity[1] = state.HeadPose.AngularVelocity.y;
		pose.vecAngularVelocity[2] = state.HeadPose.AngularVelocity.z;

		pose.poseTimeOffset = state.HeadPose.TimeInSeconds - pvrNow;

		pose.poseIsValid = true;
		pose.result = vr::TrackingResult_Running_OK;
	}
	vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(pose));
}
