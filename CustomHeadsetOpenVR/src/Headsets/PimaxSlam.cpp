#include "PimaxSlam.h"

#include <DirectXMath.h>
#include <filesystem>
#include <memory>
#include "nlohmann/json.hpp"

static constexpr uint64_t k_UniverseId = 0x50494D4158; // "PIMAX"

// A driver for Pimax Crystal controllers.
class PimaxCrystalControllerDriver : public vr::ITrackedDeviceServerDriver, public PimaxCommon {
public:
	PimaxCrystalControllerDriver(vr::ETrackedControllerRole role) : role(role) {
	}

	vr::EVRInitError Activate(uint32_t unObjectId) override {
		deviceIndex = unObjectId;

		const bool isLeft = role == vr::TrackedControllerRole_LeftHand;

		const vr::PropertyContainerHandle_t container =
			vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);

		vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, role);

		// Purely emulate an Oculus Touch controller for compatibility.
		vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "oculus");
		vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "Oculus");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_ModelNumber_String, isLeft ? "Oculus Quest2 (Left Controller)" : "Oculus Quest2 (Right Controller)");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_InputProfilePath_String, "{oculus}/input/touch_profile.json");
		vr::VRProperties()->SetStringProperty(container, vr::Prop_ControllerType_String, "oculus_touch");

		vr::VRProperties()->SetUint64Property(container, vr::Prop_CurrentUniverseId_Uint64, k_UniverseId);

		{
			using json = nlohmann::json;
			using namespace DirectX;

			std::filesystem::path renderModelPath = "{CustomHeadsetOpenVR}/rendermodels/";
			std::string renderModel = isLeft ? "crystal_controller_left" : "crystal_controller_right";
			renderModelPath /= renderModel;

			vr::VRProperties()->SetStringProperty(
				container, vr::Prop_RenderModelName_String, renderModelPath.string().c_str());

			XMMATRIX originToGrip = XMMatrixIdentity();

			// SteamVR expects the pose at the origin of the render model.
			// Retrieve the tip transform from the rendermodel, and reverse it.
			const uint32_t length = vr::VRResources()->LoadSharedResource(
				(renderModelPath / (renderModel + ".json")).string().c_str(), nullptr, 0);
			std::string content;
			content.resize(length);
			vr::VRResources()->LoadSharedResource(
				(renderModelPath / (renderModel + ".json")).string().c_str(), content.data(), length);
			try {
				const auto modelJson = json::parse(content.cbegin(), content.cend(), nullptr, true, true);
				const auto componentLocal = modelJson["components"]["tip"]["component_local"];
				const auto orientation = componentLocal["rotate_xyz"].get<std::vector<float>>();
				const auto origin = componentLocal["origin"].get<std::vector<float>>();

				poseOffset = XMMatrixRotationRollPitchYaw(
					orientation[0] * 3.1415926f / 180,
					orientation[1] * 3.1415926f / 180,
					orientation[2] * 3.1415926f / 180);
				poseOffset.r[3] = XMVectorSet(origin[0], origin[1], origin[2], 1.f);
				poseOffset = XMMatrixInverse(nullptr, poseOffset);
			}
			catch (std::exception& exc) {
				DriverLog("Error parsing render model %s: %s", renderModel.c_str(), exc.what());
			}
		}

		// Setup the icons.
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceOff_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_off.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceSearching_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_searching.gif");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_searching_alert.gif");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceReady_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_ready.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_ready_alert.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceNotReady_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_error.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceAlertLow_String, "{CustomHeadsetOpenVR}/icons/crystal_controller/controller_status_ready_low.png");

		// Create the input components.
		vr::VRDriverInput()->CreateScalarComponent(container,
			"/input/trigger/value",
			&components[ComponentTrigger],
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedOneSided);
		vr::VRProperties()->SetInt32Property(
			container, vr::Prop_Axis1Type_Int32,vr::EVRControllerAxisType::k_eControllerAxis_Trigger);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/trigger/click", &components[ComponentTriggerClick]);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/trigger/touch", &components[ComponentTriggerTouch]);
		vr::VRDriverInput()->CreateScalarComponent(container,
			"/input/grip/value",
			&components[ComponentGrip],
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedOneSided);
		vr::VRProperties()->SetInt32Property(
			container, vr::Prop_Axis3Type_Int32, vr::EVRControllerAxisType::k_eControllerAxis_Trigger);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/grip/click", &components[ComponentGripClick]);

		vr::VRDriverInput()->CreateScalarComponent(container,
			"/input/joystick/x",
			&components[ComponentThumbstickX],
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedTwoSided);
		vr::VRDriverInput()->CreateScalarComponent(container,
			"/input/joystick/y",
			&components[ComponentThumbstickY],
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedTwoSided);
		vr::VRProperties()->SetInt32Property(
			container, vr::Prop_Axis2Type_Int32, vr::EVRControllerAxisType::k_eControllerAxis_Joystick);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/joystick/click", &components[ComponentThumbstickClick]);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/joystick/touch", &components[ComponentThumbstickTouch]);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/thumbrest/touch", &components[ComponentThumbrestTouch]);

		if (isLeft) {
			vr::VRDriverInput()->CreateBooleanComponent(
				container, "/input/system/click", &components[ComponentMenu]);
		}
		vr::VRDriverInput()->CreateBooleanComponent(
			container, isLeft ? "/input/y/click" : "/input/b/click", &components[ComponentButton1]);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, isLeft ? "/input/y/touch" : "/input/b/touch", &components[ComponentButton1Touch]);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, isLeft ? "/input/x/click" : "/input/a/click", &components[ComponentButton2]);
		vr::VRDriverInput()->CreateBooleanComponent(
			container, isLeft ? "/input/x/touch" : "/input/a/touch", &components[ComponentButton2Touch]);

		vr::VRInputComponentHandle_t componentHaptics;
		vr::VRDriverInput()->CreateHapticComponent(container, "/output/haptic", &componentHaptics);

		return vr::VRInitError_None;
	}

	void Deactivate() override {
		deviceIndex = vr::k_unTrackedDeviceIndexInvalid;
	}

	void EnterStandby() override {
	}

	void* GetComponent(const char* pchComponentNameAndVersion) override {
		return nullptr;
	}

	vr::DriverPose_t GetPose() override {
		// This method is not used by SteamVR.
		return {};
	}

	void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override {
		if (unResponseBufferSize >= 1) {
			pchResponseBuffer[0] = 0;
		}
	}

	void BroadcastHapticEvent(const vr::VREvent_HapticVibration_t& data) {
		if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
			const vr::PropertyContainerHandle_t container =
				vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);

			// Only pick the events applicable to us.
			if (container == data.containerHandle) {
				pvr_triggerHapticPulse(GetPvrSession(),
					role == vr::TrackedControllerRole_LeftHand ? pvrTrackedDevice_LeftController : pvrTrackedDevice_RightController,
					data.fAmplitude,
					data.fDurationSeconds > 0.02f ? data.fDurationSeconds : 0.02f,
					data.fFrequency);
			}
		}
	}

	void UpdateInputState(const pvrInputState& inputState) {
		const auto pvrNow = GetPvrTime();
		const auto side = role == vr::TrackedControllerRole_LeftHand ? 0 : 1;

		if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
			const vr::PropertyContainerHandle_t container =
				vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);

			const auto timeOffset = inputState.TimeInSeconds - pvrNow;

			// Update the state of each input component.
			vr::VRDriverInput()->UpdateScalarComponent(
				components[ComponentTrigger], inputState.Trigger[side], timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentTriggerClick], inputState.HandButtons[side] & pvrButton_Trigger, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentTriggerTouch], inputState.HandTouches[side] & pvrButton_Trigger, timeOffset);
			vr::VRDriverInput()->UpdateScalarComponent(
				components[ComponentGrip], inputState.Grip[side], timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentGripClick], inputState.HandButtons[side] & pvrButton_Grip, timeOffset);

			vr::VRDriverInput()->UpdateScalarComponent(
				components[ComponentThumbstickX], inputState.JoyStick[side].x, timeOffset);
			vr::VRDriverInput()->UpdateScalarComponent(
				components[ComponentThumbstickY], inputState.JoyStick[side].y, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentThumbstickClick], inputState.HandButtons[side] & pvrButton_JoyStick, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentThumbstickTouch], inputState.HandTouches[side] & pvrButton_JoyStick, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentThumbrestTouch], inputState.HandTouches[side] & pvrButton_TouchPad, timeOffset);

			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentButton1], inputState.HandButtons[side] & pvrButton_B, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentButton2], inputState.HandButtons[side] & pvrButton_A, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentButton1Touch], inputState.HandTouches[side] & pvrButton_B, timeOffset);
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentButton2Touch], inputState.HandTouches[side] & pvrButton_A, timeOffset);
			if (side == 0) {
				vr::VRDriverInput()->UpdateBooleanComponent(
					components[ComponentMenu], inputState.HandButtons[side] & pvrButton_ApplicationMenu, timeOffset);
			}

			// Update the battery level.
			const int batteryPercentage = pvr_getTrackedDeviceIntProperty(
				GetPvrSession(),
				role == vr::TrackedControllerRole_LeftHand ? pvrTrackedDevice_LeftController : pvrTrackedDevice_RightController,
				pvrTrackedDeviceProp_BatteryPercent_int,
				-1);
			if (batteryPercentage > 0) {
				vr::VRProperties()->SetFloatProperty(
					container, vr::Prop_DeviceBatteryPercentage_Float, batteryPercentage / 100.f);
				vr::VRProperties()->SetBoolProperty(
					container, vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
			}
		}
	}

	void UpdateTrackingState(const pvrPoseStatef& poseState) {
		const auto pvrNow = GetPvrTime();

		vr::DriverPose_t pose = {};
		pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
		if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
			pose.deviceIsConnected = true;
			pose.result = vr::TrackingResult_Running_OutOfRange;

			if (poseState.StatusFlags & pvrStatus_OrientationTracked) {
				pose.vecPosition[0] = poseState.ThePose.Position.x;
				pose.vecPosition[1] = poseState.ThePose.Position.y;
				pose.vecPosition[2] = poseState.ThePose.Position.z;
				pose.qRotation.x = poseState.ThePose.Orientation.x;
				pose.qRotation.y = poseState.ThePose.Orientation.y;
				pose.qRotation.z = poseState.ThePose.Orientation.z;
				pose.qRotation.w = poseState.ThePose.Orientation.w;

				pose.vecVelocity[0] = poseState.LinearVelocity.x;
				pose.vecVelocity[1] = poseState.LinearVelocity.y;
				pose.vecVelocity[2] = poseState.LinearVelocity.z;
				pose.vecAngularVelocity[0] = poseState.AngularVelocity.x;
				pose.vecAngularVelocity[1] = poseState.AngularVelocity.y;
				pose.vecAngularVelocity[2] = poseState.AngularVelocity.z;

				// Add the offset to align with the controller model.
				DirectX::XMVECTOR position, orientation, scale;
				DirectX::XMMatrixDecompose(&scale, &orientation, &position, poseOffset);
				pose.vecDriverFromHeadTranslation[0] = DirectX::XMVectorGetX(position);
				pose.vecDriverFromHeadTranslation[1] = DirectX::XMVectorGetY(position);
				pose.vecDriverFromHeadTranslation[2] = DirectX::XMVectorGetZ(position);
				pose.qDriverFromHeadRotation.x = DirectX::XMVectorGetX(orientation);
				pose.qDriverFromHeadRotation.y = DirectX::XMVectorGetY(orientation);
				pose.qDriverFromHeadRotation.z = DirectX::XMVectorGetZ(orientation);
				pose.qDriverFromHeadRotation.w = DirectX::XMVectorGetW(orientation);

				pose.poseTimeOffset = poseState.TimeInSeconds - pvrNow;

				pose.poseIsValid = true;
				pose.result = vr::TrackingResult_Running_OK;
			}
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(pose));
		}
	}

	void Disconnect() {
		vr::DriverPose_t pose = {};
		pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
		pose.result = vr::TrackingResult_Running_OutOfRange;
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(pose));
	}

private:
	enum Components {
		ComponentTrigger,
		ComponentTriggerClick,
		ComponentTriggerTouch,
		ComponentGrip,
		ComponentGripClick,
		ComponentThumbstickX,
		ComponentThumbstickY,
		ComponentThumbstickClick,
		ComponentThumbstickTouch,
		ComponentThumbrestTouch,
		ComponentButton1,
		ComponentButton1Touch,
		ComponentButton2,
		ComponentButton2Touch,
		ComponentMenu,

		ComponentCount,
	};

	const vr::ETrackedControllerRole role;
	vr::TrackedDeviceIndex_t deviceIndex = vr::k_unTrackedDeviceIndexInvalid;
	vr::VRInputComponentHandle_t components[ComponentCount] = {};

	DirectX::XMMATRIX poseOffset = DirectX::XMMatrixIdentity();
};

static std::unique_ptr<PimaxCrystalControllerDriver> s_controllerDriver[2];

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

	eyeTracking.eyeRotation = defaultDriverConfig.dreamAir.eyeRotation;
	eyeTracking.enabled = HasEyeTracking() && GetConfig().enableEyeTracking;
	eyeTracking.Initialize();

	returnValue = vr::VRInitError_None;
	BaseHeadsetShim::PosTrackedDeviceActivate(unObjectId, returnValue);
}

void PimaxSlamDriver::SubDeactivate() {
	eyeTracking.Shutdown();
}

void PimaxSlamDriver::SubRunFrame() {
	const auto pvrNow = GetPvrTime();

	pvrTrackingState trackingState = {};
	pvr_getTrackingState(GetPvrSession(), pvrNow, &trackingState);

	// Update the headset pose.
	{
		vr::DriverPose_t pose = {};
		pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
		pose.deviceIsConnected = true;
		pose.result = vr::TrackingResult_Running_OutOfRange;
		if (trackingState.HeadPose.StatusFlags & pvrStatus_OrientationTracked) {
			pose.vecPosition[0] = trackingState.HeadPose.ThePose.Position.x;
			pose.vecPosition[1] = trackingState.HeadPose.ThePose.Position.y;
			pose.vecPosition[2] = trackingState.HeadPose.ThePose.Position.z;
			pose.qRotation.x = trackingState.HeadPose.ThePose.Orientation.x;
			pose.qRotation.y = trackingState.HeadPose.ThePose.Orientation.y;
			pose.qRotation.z = trackingState.HeadPose.ThePose.Orientation.z;
			pose.qRotation.w = trackingState.HeadPose.ThePose.Orientation.w;

			pose.vecVelocity[0] = trackingState.HeadPose.LinearVelocity.x;
			pose.vecVelocity[1] = trackingState.HeadPose.LinearVelocity.y;
			pose.vecVelocity[2] = trackingState.HeadPose.LinearVelocity.z;

			pose.vecAngularVelocity[0] = trackingState.HeadPose.AngularVelocity.x;
			pose.vecAngularVelocity[1] = trackingState.HeadPose.AngularVelocity.y;
			pose.vecAngularVelocity[2] = trackingState.HeadPose.AngularVelocity.z;

			pose.poseTimeOffset = trackingState.HeadPose.TimeInSeconds - pvrNow;

			pose.poseIsValid = true;
			pose.result = vr::TrackingResult_Running_OK;
		}
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(pose));
	}

	pvrInputState inputState = {};
	pvr_getInputState(GetPvrSession(), &inputState);

	for (uint32_t side = 0; side < 2; side++) {
		// Detect controllers.
		const auto controllerType = pvr_getTrackedDeviceStringPropertyHelper(
			GetPvrSession(),
			side == 0 ? pvrTrackedDevice_LeftController : pvrTrackedDevice_RightController,
			pvrTrackedDeviceProp_ControllerType_String);
		const bool isActive = !controllerType.empty();
		if (isActive) {
			if (!s_controllerDriver[side]) {
				const bool isPimaxController = controllerType == "pimax_crystal";
				if (isPimaxController) {
					s_controllerDriver[side] = std::make_unique<PimaxCrystalControllerDriver>(
						side == 0 ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);

					vr::VRServerDriverHost()->TrackedDeviceAdded(
						side == 0 ? "PIMAXLEFT" : "PIMAXRIGHT", vr::TrackedDeviceClass_Controller, s_controllerDriver[side].get());
				}
			}
		}
		else if (s_controllerDriver[side]) {
			s_controllerDriver[side]->Disconnect();
		}

		// Update controller poses and button states.
		if (s_controllerDriver[side]) {
			s_controllerDriver[side]->UpdateTrackingState(trackingState.HandPoses[side]);
			s_controllerDriver[side]->UpdateInputState(inputState);
		}
	}

	// Update eye tracking.
	eyeTracking.ipd = GetConfig().ipd + GetConfig().ipdOffset;
	eyeTracking.enabled = HasEyeTracking() && GetConfig().enableEyeTracking;
	// eyeTracking.eyeRotation = GetConfig().eyeRotation;
	eyeTracking.RunFrame();
}

void PimaxSlamDriver::HandleEvent(const vr::VREvent_t& event) {
	switch (event.eventType) {
	case vr::VREvent_Input_HapticVibration:
		for (uint32_t side = 0; side < 2; side++) {
			if (s_controllerDriver[side]) {
				s_controllerDriver[side]->BroadcastHapticEvent(event.data.hapticVibration);
			}
		}
		break;
	}
}
