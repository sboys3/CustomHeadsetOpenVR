#include "PimaxSlam.h"

#include "../Helpers/EyeTrackingOutput.h"
#include "../Helpers/vr_blockqueue.h"

#include <DirectXMath.h>
#include <filesystem>
#include <memory>
#include "nlohmann/json.hpp"
#include <PVR_Math.h>
#include <shared_mutex>

// Matches driver_aapvr.
static constexpr uint64_t k_UniverseId = 30;

static inline vr::HmdMatrix34_t StoreHmdMatrix34(const PVR::Matrix4f& matrix) {
	return { matrix.M[0][0],
			matrix.M[0][1],
			matrix.M[0][2],
			matrix.M[0][3],
			matrix.M[1][0],
			matrix.M[1][1],
			matrix.M[1][2],
			matrix.M[1][3],
			matrix.M[2][0],
			matrix.M[2][1],
			matrix.M[2][2],
			matrix.M[2][3] };
}

static inline vr::HmdMatrix34_t StoreHmdMatrix34(const DirectX::XMMATRIX& matrix) {
	DirectX::XMFLOAT4X3 temp;
	DirectX::XMStoreFloat4x3(&temp, matrix);
	return { temp.m[0][0],
			temp.m[1][0],
			temp.m[2][0],
			temp.m[3][0],
			temp.m[0][1],
			temp.m[1][1],
			temp.m[2][1],
			temp.m[3][1],
			temp.m[0][2],
			temp.m[1][2],
			temp.m[2][2],
			temp.m[3][2] };
}

// A driver for Pimax Crystal controllers.
class PimaxCrystalControllerDriver : public vr::ITrackedDeviceServerDriver {
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
		if(k_UniverseId){
			vr::VRProperties()->SetUint64Property(container, vr::Prop_CurrentUniverseId_Uint64, k_UniverseId);
		}

		{
			using json = nlohmann::json;
			using namespace DirectX;

			std::filesystem::path renderModelPath = "{aapvr}/rendermodels/";
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
			container, vr::Prop_NamedIconPathDeviceOff_String, "{aapvr}/icons/crystal_controller_status_off.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceSearching_String, "{aapvr}/icons/crystal_controller_status_searching.gif");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{aapvr}/icons/crystal_controller_status_searching_alert.gif");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceReady_String, "{aapvr}/icons/crystal_controller_status_ready.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{aapvr}/icons/crystal_controller_status_ready_alert.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceNotReady_String, "{aapvr}/icons/crystal_controller_status_error.png");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_NamedIconPathDeviceAlertLow_String, "{aapvr}/icons/crystal_controller_status_ready_low.png");

		// Create the input components.
		vr::VRDriverInput()->CreateScalarComponent(container,
			"/input/trigger/value",
			&components[ComponentTrigger],
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedOneSided);
		vr::VRProperties()->SetInt32Property(
			container, vr::Prop_Axis1Type_Int32, vr::EVRControllerAxisType::k_eControllerAxis_Trigger);
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

		// The input profile does not support the system button on the right hand, however it still appears to work
		// in SteamVR to invoke the dashboard. It's a happy accident.
		vr::VRDriverInput()->CreateBooleanComponent(
			container, "/input/system/click", &components[ComponentMenu]);

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
				pvr_triggerHapticPulse(PimaxCommon::GetPvrSession(),
					role == vr::TrackedControllerRole_LeftHand ? pvrTrackedDevice_LeftController : pvrTrackedDevice_RightController,
					data.fAmplitude,
					data.fDurationSeconds > 0.02f ? data.fDurationSeconds : 0.02f,
					data.fFrequency);
			}
		}
	}

	void UpdateInputState(const pvrInputState& inputState) {
		const auto pvrNow = PimaxCommon::GetPvrTime();
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
			vr::VRDriverInput()->UpdateBooleanComponent(
				components[ComponentMenu], inputState.HandButtons[side] & pvrButton_ApplicationMenu, timeOffset);

			// Update the battery level.
			const int batteryPercentage = pvr_getTrackedDeviceIntProperty(
				PimaxCommon::GetPvrSession(),
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
		const auto pvrNow = PimaxCommon::GetPvrTime();

		vr::DriverPose_t pose = {};
		pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
		if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
			isConnected = isConnected || (poseState.StatusFlags & pvrStatus_OrientationTracked);
			pose.deviceIsConnected = isConnected;
			pose.result = vr::TrackingResult_Running_OutOfRange;

			if (isConnected) {
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
		isConnected = false;
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
	bool isConnected = false;

	DirectX::XMMATRIX poseOffset = DirectX::XMMatrixIdentity();
};

// A driver for Pimax Video See Through (VST).
// IVRCameraComponent is not officially documented.
// See https://github.com/Rectus/openvr_camera_sim/blob/main/camera_component.cpp
class PimaxCameraDriver : public vr::IVRCameraComponent {
public:
	PimaxCameraDriver() {
		QueryPerformanceFrequency(&qpcFrequency);
	}

	void Activate(vr::TrackedDeviceIndex_t deviceIndex) {
		const vr::PropertyContainerHandle_t container =
			vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);

		numCameras = pvr_getVSTType(PimaxCommon::GetPvrSession()) == pvrVSTTypeStereo ? 2 : 1;
		for (uint32_t i = 0; i < numCameras; i++) {
			pvr_getVSTCameraIntrinsics(PimaxCommon::GetPvrSession(), i, &cameraResolutionWidth, &cameraResolutionHeight, &focalLength[i], &principalPoint[i]);
			pvr_getVSTCameraExtrinsics(PimaxCommon::GetPvrSession(), i, &cameraToHmd[i]);
			pvrVSTDistortionType distortionType = {};
			pvr_getVSTCameraDistortionParams(PimaxCommon::GetPvrSession(), i, &distortionType, distortionParams[i]);
		}

		vr::VRProperties()->SetBoolProperty(container, vr::Prop_HasCamera_Bool, true);
		vr::VRProperties()->SetStringProperty(container, vr::Prop_CameraFirmwareDescription_String, "Pimax VST");
		vr::VRProperties()->SetInt32Property(container, vr::Prop_NumCameras_Int32, numCameras);
		vr::VRProperties()->SetInt32Property(
			container,
			vr::Prop_CameraFrameLayout_Int32,
			numCameras == 1
			? vr::EVRTrackedCameraFrameLayout_Mono
			: (vr::EVRTrackedCameraFrameLayout_Stereo | vr::EVRTrackedCameraFrameLayout_HorizontalLayout));
		vr::VRProperties()->SetInt32Property(container, vr::Prop_CameraStreamFormat_Int32, vr::CVS_FORMAT_NV12);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_CameraSupportsCompatibilityModes_Bool, false);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_AllowCameraToggle_Bool, true);

		std::vector<vr::HmdMatrix34_t> transforms;
		for (uint32_t i = 0; i < numCameras; i++) {
			transforms.push_back(StoreHmdMatrix34(PVR::Matrix4(PVR::Posef(cameraToHmd[i]))));
		}
		vr::VRProperties()->SetProperty(container,
			vr::Prop_CameraToHeadTransform_Matrix34,
			&transforms[0],
			sizeof(vr::HmdMatrix34_t),
			vr::k_unHmdMatrix34PropertyTag);
		vr::VRProperties()->SetPropertyVector(
			container, vr::Prop_CameraToHeadTransforms_Matrix34_Array, vr::k_unHmdMatrix34PropertyTag, &transforms);

		std::vector<int32_t> distortionFunction;
		distortionFunction.push_back((int32_t)vr::VRDistortionFunctionType_Extended_FTheta);
		distortionFunction.push_back((int32_t)vr::VRDistortionFunctionType_Extended_FTheta);
		vr::VRProperties()->SetPropertyVector(container,
			vr::Prop_CameraDistortionFunction_Int32_Array,
			vr::k_unInt32PropertyTag,
			&distortionFunction);
		std::vector<double> distortionCoeff;
		for (uint32_t i = 0; i < numCameras; i++) {
			for (uint32_t j = 0; j < std::size(distortionParams[0]); j++) {
				distortionCoeff.push_back(distortionParams[i][j]);
			}
		}
		vr::VRProperties()->SetPropertyVector(container,
			vr::Prop_CameraDistortionCoefficients_Float_Array,
			vr::k_unFloatPropertyTag,
			&distortionCoeff);

		{
			uint32_t nFrameBufferDataSize = 0;
			int nDefaultFrameQueueSize = 0;
			vr::ECameraVideoStreamFormat nVideoStreamFormat = GetCameraVideoStreamFormat();
			GetCameraFrameBufferingRequirements(&nDefaultFrameQueueSize, &nFrameBufferDataSize);
			vr::VRBlockQueue()->Create(&cameraBlockQueue,
				"/lighthouse/camera/raw_frames",
				nFrameBufferDataSize,
				512,
				nDefaultFrameQueueSize,
				0);
			DriverLog("Camera Block Queue: %lld", cameraBlockQueue);

			vr::VRPathsSet<UINT32>(
				cameraBlockQueue, vr::k_pathCameraWidth, cameraResolutionWidth * numCameras, vr::k_unInt32PropertyTag);
			vr::VRPathsSet<UINT32>(
				cameraBlockQueue, vr::k_pathCameraHeight, cameraResolutionHeight, vr::k_unInt32PropertyTag);
			vr::VRPathsSet<UINT32>(
				cameraBlockQueue, vr::k_pathCameraFormat, nVideoStreamFormat, vr::k_unInt32PropertyTag);
		}

		// Force RoomView by faking firmware versions that will pass the checks in vrcompositor.
		vr::VRProperties()->SetUint64Property(container, vr::Prop_FPGAVersion_Uint64, UINT64_MAX);
		vr::VRProperties()->SetUint64Property(container, vr::Prop_FirmwareVersion_Uint64, UINT64_MAX);
		vr::VRProperties()->SetUint64Property(container, vr::Prop_CameraFirmwareVersion_Uint64, UINT64_MAX);

		vr::VRProperties()->SetBoolProperty(container, vr::Prop_SupportsRoomViewDepthProjection_Bool, false);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_AllowLightSourceFrequency_Bool, false);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_Hmd_SupportsRoomViewDirect_Bool, false);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_CameraSupportsCompatibilityModes_Bool, false);
		vr::VRProperties()->SetInt32Property(container, vr::Prop_CameraCompatibilityMode_Int32, 0);
		{
			std::vector<float> CameraWhiteBalance;
			CameraWhiteBalance.resize(8);
			CameraWhiteBalance[0] = 1.0f;
			CameraWhiteBalance[1] = 1.0f;
			CameraWhiteBalance[2] = 1.0f;
			CameraWhiteBalance[4] = 1.0f;
			CameraWhiteBalance[5] = 1.0f;
			CameraWhiteBalance[6] = 1.0f;
			vr::VRProperties()->SetPropertyVector(container,
				vr::Prop_CameraWhiteBalance_Vector4_Array,
				vr::k_unHmdVector4PropertyTag,
				&CameraWhiteBalance);
		}
	}

	bool GetCameraFrameDimensions(vr::ECameraVideoStreamFormat nVideoStreamFormat, uint32_t* pWidth, uint32_t* pHeight) override {
		bool status = false;
		if (nVideoStreamFormat == vr::CVS_FORMAT_NV12 || nVideoStreamFormat == vr::CVS_FORMAT_UNKNOWN) {
			*pWidth = cameraResolutionWidth * numCameras;
			*pHeight = cameraResolutionHeight;
			status = true;
		}

		return status;
	}

	bool GetCameraFrameBufferingRequirements(int* pDefaultFrameQueueSize, uint32_t* pFrameBufferDataSize) override {
		// Assume NV12.
		uint32_t nFrameBufferDataSize = (cameraResolutionWidth * cameraResolutionHeight * numCameras * 3) / 2;
		nFrameBufferDataSize = sizeof(vr::CameraVideoStreamFrame_t) + nFrameBufferDataSize;
		nFrameBufferDataSize = ((nFrameBufferDataSize + 15) / 16) * 16;
		if (pDefaultFrameQueueSize) {
			*pDefaultFrameQueueSize = 2;
		}
		if (pFrameBufferDataSize) {
			*pFrameBufferDataSize = nFrameBufferDataSize;
		}

		return true;
	}

	bool SetCameraFrameBuffering(int nFrameBufferCount, void** ppFrameBuffers, uint32_t nFrameBufferDataSize) override {
		bool status = false;
		if (nFrameBufferCount == 0) {
			cameraBuffer[0] = cameraBuffer[1] = nullptr;
			status = true;
		}
		else if (nFrameBufferCount > 1) {
			for (uint32_t i = 0; i < 2; i++) {
				cameraBuffer[i] = (vr::CameraVideoStreamFrame_t*)ppFrameBuffers[i];
				*cameraBuffer[i] = {};
				cameraBuffer[i]->m_flFrameDeliveryRate = 1 / 30.0;
				cameraBuffer[i]->m_nBufferCount = 2;
				cameraBuffer[i]->m_nBufferIndex = (uint32_t)i;
				cameraBuffer[i]->m_nWidth = cameraResolutionWidth * numCameras;
				cameraBuffer[i]->m_nHeight = cameraResolutionHeight;
				cameraBuffer[i]->m_nStreamFormat = vr::CVS_FORMAT_NV12;
				cameraBuffer[i]->m_nImageDataSize =
					(cameraResolutionWidth * cameraResolutionHeight * numCameras * 3) / 2;
				cameraBuffer[i]->m_pImageData = (uint64_t)(cameraBuffer[i] + 1);
				cameraBuffer[i]->m_RawTrackedDevicePose.bDeviceIsConnected = true;
				cameraBuffer[i]->m_RawTrackedDevicePose.bPoseIsValid = false;
				cameraBuffer[i]->m_RawTrackedDevicePose.mDeviceToAbsoluteTracking =
					StoreHmdMatrix34(DirectX::XMMatrixIdentity());
				cameraBuffer[i]->m_RawTrackedDevicePose.eTrackingResult = vr::TrackingResult_Running_OK;
			}
			status = true;
		}

		return status;
	}

	bool SetCameraVideoStreamFormat(vr::ECameraVideoStreamFormat nVideoStreamFormat) override {
		bool status = false;
		if (nVideoStreamFormat == vr::CVS_FORMAT_NV12) {
			status = true;
		}

		return status;
	}

	vr::ECameraVideoStreamFormat GetCameraVideoStreamFormat() override {
		return vr::CVS_FORMAT_NV12;
	}

	bool StartVideoStream() override {
		QueryPerformanceCounter(&cameraStartTime);
		cameraFrameIndex = 0;
		cameraActive = true;
		cameraPaused = false;

		return true;
	}

	void StopVideoStream() override {
		cameraActive = cameraPaused = false;
	}

	bool IsVideoStreamActive(bool* pbPaused, float* pflElapsedTime) override {
		*pbPaused = cameraActive && cameraPaused;
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		*pflElapsedTime =
			cameraActive ? (now.QuadPart - cameraStartTime.QuadPart) / (float)qpcFrequency.QuadPart : 0.f;

		return cameraActive;
	}

	const vr::CameraVideoStreamFrame_t* GetVideoStreamFrame() override {
		// Not supported.
		return nullptr;
	}

	void ReleaseVideoStreamFrame(const vr::CameraVideoStreamFrame_t* pFrameImage) override {
	}

	bool SetAutoExposure(bool bEnable) override {
		// Not supported.
		return false;
	}

	bool PauseVideoStream() override {
		cameraPaused = true;
		return true;
	}

	bool ResumeVideoStream() override {
		cameraPaused = false;
		return true;
	}

	bool GetCameraDistortion(uint32_t nCameraIndex, float flInputU, float flInputV, float* pflOutputU, float* pflOutputV) override {
		// Taken as-is from https://github.com/Rectus/openvr_camera_sim/blob/main/camera_component.cpp
		const double focalX = focalLength[nCameraIndex].x / (double)cameraResolutionWidth;
		const double focalY = focalLength[nCameraIndex].y / (double)cameraResolutionHeight;

		const double centerX = principalPoint[nCameraIndex].x / (double)cameraResolutionWidth - 0.5;
		const double centerY = principalPoint[nCameraIndex].y / (double)cameraResolutionHeight - 0.5;

		double UScaled = (flInputU - 0.5) * 2.0 / focalX;
		double VScaled = (flInputV - 0.5) * 2.0 / focalY;

		double radius = sqrt(UScaled * UScaled + VScaled * VScaled);

		double theta = atan(radius);

		double thetaD = theta + distortionParams[nCameraIndex][0] * pow(theta, 3) +
			distortionParams[nCameraIndex][1] * pow(theta, 5) +
			distortionParams[nCameraIndex][2] * pow(theta, 7) +
			distortionParams[nCameraIndex][3] * pow(theta, 9);

		double radialFactor = thetaD / radius;

		*pflOutputU = (float)(UScaled * radialFactor * focalX + centerX + 0.5);
		*pflOutputV = (float)(VScaled * radialFactor * focalY + centerY + 0.5);

		return true;
	}

	bool GetCameraProjection(uint32_t nCameraIndex, vr::EVRTrackedCameraFrameType eFrameType, float flZNear, float flZFar, vr::HmdMatrix44_t* pProjection) override {
		// Taken as-is from https://github.com/Rectus/openvr_camera_sim/blob/main/camera_component.cpp
		*pProjection = {};
		pProjection->m[0][0] = focalLength[nCameraIndex].x / (float)cameraResolutionWidth / 2.0f;
		pProjection->m[1][1] = focalLength[nCameraIndex].y / (float)cameraResolutionHeight;
		pProjection->m[0][2] = principalPoint[nCameraIndex].x / (float)cameraResolutionWidth;
		pProjection->m[1][2] = principalPoint[nCameraIndex].y / (float)cameraResolutionHeight - 0.5f;
		pProjection->m[2][2] = -flZFar / (flZFar - flZNear);
		pProjection->m[2][3] = -flZFar * flZNear / (flZFar - flZNear);
		pProjection->m[3][2] = -1;

		return true;
	}

	bool SetFrameRate(int nISPFrameRate, int nSensorFrameRate) override {
		// We cannot service this request. Pretend success.
		return true;
	}

	bool SetCameraVideoSinkCallback(vr::ICameraVideoSinkCallback* pCameraVideoSinkCallback) override {
		cameraSinkCallback = pCameraVideoSinkCallback;
		return true;
	}

	bool GetCameraCompatibilityMode(vr::ECameraCompatibilityMode* pCameraCompatibilityMode) override {
		*pCameraCompatibilityMode = vr::CAMERA_COMPAT_MODE_ISO_30FPS;
		return true;
	}

	bool SetCameraCompatibilityMode(vr::ECameraCompatibilityMode nCameraCompatibilityMode) override {
		bool status = false;
		if (nCameraCompatibilityMode == vr::CAMERA_COMPAT_MODE_ISO_30FPS) {
			status = true;
		}

		return status;
	}

	bool GetCameraFrameBounds(vr::EVRTrackedCameraFrameType eFrameType, uint32_t* pLeft, uint32_t* pTop, uint32_t* pWidth, uint32_t* pHeight) override {
		*pLeft = *pTop = 0;
		*pWidth = cameraResolutionWidth * numCameras;
		*pHeight = cameraResolutionHeight;

		return true;
	}

	bool GetCameraIntrinsics(uint32_t nCameraIndex, vr::EVRTrackedCameraFrameType eFrameType, vr::HmdVector2_t* pFocalLength, vr::HmdVector2_t* pCenter, vr::EVRDistortionFunctionType* peDistortionType, double rCoefficients[vr::k_unMaxDistortionFunctionParameters]) override {
		(*pFocalLength).v[0] = focalLength[nCameraIndex].x;
		(*pFocalLength).v[1] = focalLength[nCameraIndex].y;

		(*pCenter).v[0] = principalPoint[nCameraIndex].x;
		(*pCenter).v[1] = principalPoint[nCameraIndex].y;

		*peDistortionType = vr::EVRDistortionFunctionType::VRDistortionFunctionType_FTheta;
		for (uint32_t i = 0; i < std::size(distortionParams[0]); i++) {
			rCoefficients[i] = distortionParams[nCameraIndex][i];
		}

		return true;
	}

	void RunFrame() {
		if (cameraActive && !cameraPaused && cameraBuffer[cameraBufferIndex]) {
			// Flip buffer.
			cameraBufferIndex ^= 1;

			// Retrieve and convert the image from PVR.
			const uint32_t frameSize = (cameraResolutionWidth * cameraResolutionHeight * numCameras * 3) / 2;
			pvrVSTStreamFrame frame = {};
			pvr_getVSTStreamFrame(PimaxCommon::GetPvrSession(), pvrFrameIndex, &frame);
			pvrFrameIndex = frame.frameIdx + 1;

			switch (pvr_getVSTStreamFormat(PimaxCommon::GetPvrSession())) {
			case pvrVST_FORMAT_RAW8:
			{
				uint8_t* y_plane = (uint8_t*)cameraBuffer[cameraBufferIndex]->m_pImageData;
				memcpy((uint8_t*)cameraBuffer[cameraBufferIndex]->m_pImageData,
					frame.buffer,
					cameraResolutionWidth * cameraResolutionHeight * numCameras);
				uint8_t* uv_plane = (uint8_t*)cameraBuffer[cameraBufferIndex]->m_pImageData +
					cameraResolutionWidth * cameraResolutionHeight * numCameras;
				memset(uv_plane, 128, (cameraResolutionWidth * cameraResolutionHeight * numCameras) / 2);
			}
			break;
			case pvrVST_FORMAT_NV12:
				memcpy((uint8_t*)cameraBuffer[cameraBufferIndex]->m_pImageData, frame.buffer, frameSize);
				break;
			}

			// Push the frame to SteamVR.
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);
			cameraBuffer[cameraBufferIndex]->m_flFrameElapsedTime =
				(now.QuadPart - cameraStartTime.QuadPart) / (double)qpcFrequency.QuadPart;
			cameraBuffer[cameraBufferIndex]->m_nFrameSequence = (uint32_t)cameraFrameIndex;

			void* pvBuffer = nullptr;
			vr::PropertyContainerHandle_t ulBlockContainer = 0;
			const bool				acquired =
				vr::VRBlockQueue()->AcquireWriteOnlyBlock(cameraBlockQueue, &ulBlockContainer, &pvBuffer) ==
				vr::EBlockQueueError_BlockQueueError_None;
			if (acquired) {
				memcpy(pvBuffer, (void*)cameraBuffer[cameraBufferIndex]->m_pImageData, frameSize);
				vr::VRPathsSet<UINT32>(
					ulBlockContainer, vr::k_pathCameraFrameSize, frameSize, vr::k_unInt32PropertyTag);

				vr::VRPathsSet<UINT64>(
					ulBlockContainer, vr::k_pathCameraFrameSequence, cameraFrameIndex, vr::k_unUint64PropertyTag);

				vr::VRPathsSet<double>(ulBlockContainer,
					vr::k_pathCameraFrameTimeMonotonic,
					now.QuadPart / (double)qpcFrequency.QuadPart,
					vr::k_unDoublePropertyTag);
				vr::VRPathsSet<UINT64>(ulBlockContainer,
					vr::k_pathCameraFrameServerTimeTicks,
					now.QuadPart,
					vr::k_unUint64PropertyTag);
				vr::VRPathsSet<double>(
					ulBlockContainer, vr::k_pathCameraFrameDeliveryRate, 1 / 30.0, vr::k_unDoublePropertyTag);
				vr::VRPathsSet<double>(ulBlockContainer,
					vr::k_pathCameraFrameElapsedTime,
					cameraBuffer[cameraBufferIndex]->m_flFrameElapsedTime,
					vr::k_unDoublePropertyTag);

				vr::VRBlockQueue()->ReleaseWriteOnlyBlock(cameraBlockQueue, ulBlockContainer);
			}

			cameraFrameIndex++;

			if (cameraSinkCallback) {
				cameraSinkCallback->OnCameraVideoSinkCallback();
			}
		}
	}

private:
	LARGE_INTEGER qpcFrequency = {};

	uint32_t numCameras = 0;
	uint32_t cameraResolutionWidth = 0;
	uint32_t cameraResolutionHeight = 0;
	pvrVector2f focalLength[2] = {};
	pvrVector2f principalPoint[2] = {};
	pvrPosef cameraToHmd[2] = {};
	float distortionParams[2][8] = {};

	uint32_t pvrFrameIndex = 0;

	vr::PropertyContainerHandle_t cameraBlockQueue = vr::k_ulInvalidPropertyContainer;
	bool cameraActive = false;
	bool cameraPaused = false;
	LARGE_INTEGER cameraStartTime = {};
	uint64_t cameraFrameIndex = 0;

	vr::CameraVideoStreamFrame_t* cameraBuffer[2] = { nullptr, nullptr };
	std::atomic<size_t> cameraBufferIndex = 1;
	vr::ICameraVideoSinkCallback* cameraSinkCallback = nullptr;
};

static std::unique_ptr<PimaxCrystalControllerDriver> s_controllerDriver[2];
static std::shared_mutex s_controllerDriverMutex;
static std::unique_ptr<PimaxCameraDriver> s_cameraDriver;

PimaxSlamDriver::PimaxSlamDriver() {
	// Setting up the camera component must be done early, prior to any GetComponent().
	const auto vstType = pvr_getVSTType(GetPvrSession());
	if (vstType != pvrVSTTypeNone) {
		const auto format = pvr_getVSTStreamFormat(GetPvrSession());
		if (format == pvrVST_FORMAT_NV12 || format == pvrVST_FORMAT_RAW8) {
			s_cameraDriver = std::make_unique<PimaxCameraDriver>();
		}
	}
}

bool PimaxSlamDriver::IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) {
	return true;
}

Config::BaseHeadsetConfig& PimaxSlamDriver::GetConfig(){
	return GetHeadsetConfig();
}

Config::BaseHeadsetConfig& PimaxSlamDriver::GetConfigOld(){
	return GetHeadsetConfigOld();
}

void PimaxSlamDriver::PosTrackedDeviceGetComponent(const char*& pchComponentNameAndVersion, void*& returnComponent) {
	BaseHeadsetShim::PosTrackedDeviceGetComponent(pchComponentNameAndVersion, returnComponent);
	if (strcmp(pchComponentNameAndVersion, vr::IVRCameraComponent_Version) == 0 && s_cameraDriver) {
		returnComponent = s_cameraDriver.get();
	}
}

void PimaxSlamDriver::PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue) {
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

	vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "aapvr");
	vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "Pimax");
	vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, GetHmdInfo().ProductName);
	vr::VRProperties()->SetStringProperty(container, vr::Prop_RenderModelName_String, "generic_hmd");
	if(k_UniverseId){
		vr::VRProperties()->SetUint64Property(container, vr::Prop_CurrentUniverseId_Uint64, k_UniverseId);
	}

	vr::VRProperties()->SetStringProperty(
		container, vr::Prop_InputProfilePath_String, ("{" + driverConfigLoader.info.driverName + "}/input/pimaxhmd_profile.json").c_str());
	vr::VRDriverInput()->CreateBooleanComponent(
		container, "/input/system/click", &inputComponents[ComponentSystemClick]);
	vr::VRDriverInput()->CreateBooleanComponent(container, "/input/tap/click", &inputComponents[ComponentTap]);

	// We need to set this config value before UpdateSettings() runs.
	// This is only necessary when using the PimaxDistortionProfile.
	pvr_setIntConfig(GetPvrSession(), "view_rotation_fix", GetConfig().parallelProjection);

	// We need to set the mesh values before UpdateSettings() runs.
	if (GetConfig().hiddenArea.enable && GetConfig().hiddenArea.autoHiddenArea) {
		SetVisibilityMeshes();
	}

	if (HasEyeTracking()) {
		eyeTrackingOutput.Initialize();
	}

	if (s_cameraDriver) {
		s_cameraDriver->Activate(unObjectId);
	}

	returnValue = vr::VRInitError_None;
	BaseHeadsetShim::PosTrackedDeviceActivate(unObjectId, returnValue);
}

void PimaxSlamDriver::SubDeactivate() {
	StopPvrTracking();
	StopEyeTracking();
}

void PimaxSlamDriver::RunFrame() {
	if(!isActive){
		// don't do anything if not the active device
		return;
	}
	// We need to set this config value before UpdateSettings() runs.
	// This is only necessary when using the PimaxDistortionProfile.
	pvr_setIntConfig(GetPvrSession(), "view_rotation_fix", GetConfig().parallelProjection);

	if (driverConfig.hasBeenUpdated &&
		(GetConfig().hiddenArea != GetConfigOld().hiddenArea || GetConfigOld().disableEye != GetConfig().disableEye)) {
		// We need to set the mesh values before UpdateSettings() runs.
		if (GetConfig().hiddenArea.enable && GetConfig().hiddenArea.autoHiddenArea) {
			SetVisibilityMeshes();
		}
	}

	BaseHeadsetShim::RunFrame();

	// Make sure to run BaseHeadsetShim::RunFrame() for housekeeping before checking for lost connection.
	if (CheckDeviceLost()) {
		StopPvrTracking();
		return;
	}

	StartPvrTracking();

	// Update controller state and buttons.
	pvrInputState inputState = {};
	pvr_getInputState(GetPvrSession(), &inputState);

	for (uint32_t side = 0; side < 2; side++) {
		// Detect controllers.
		const auto controllerType = pvr_getTrackedDeviceStringPropertyHelper(
			GetPvrSession(),
			side == 0 ? pvrTrackedDevice_LeftController : pvrTrackedDevice_RightController,
			pvrTrackedDeviceProp_ControllerType_String);
		const bool isActive = !controllerType.empty();
		{
			// Protects creation of s_controllerDriver[] and setting the isConnected flag inside PimaxCrystalControllerDriver.
			std::unique_lock lock(s_controllerDriverMutex);

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
		}

		if (s_controllerDriver[side]) {
			s_controllerDriver[side]->UpdateInputState(inputState);
		}
	}

	// Update eye tracking.
	if (HasEyeTracking() && GetConfig().enableEyeTracking) {
		StartEyeTracking();
	}
	else {
		StopEyeTracking();
	}
	eyeTrackingOutput.ipd = (float)(GetConfig().ipd + GetConfig().ipdOffset);
	eyeTrackingOutput.RunFrame();

	// Update proximity sensor.
	pvrHmdStatus hmdStatus = {};
	if(GetConfig().proximitySensorType == Config::ProximitySensorType::ProximitySensorTypeHardware){
		pvrHmdStatus hmdStatus = {};
		pvr_getHmdStatus(GetPvrSession(), &hmdStatus);
		if(inputComponents[ComponentProximity]){
			vr::VRDriverInput()->UpdateBooleanComponent(inputComponents[ComponentProximity], hmdStatus.HmdMounted, 0);
		}
	}
	// Update the buttons state.
	bool systemClick = false, doubleTap = false;
	GetHmdButtonsState(systemClick, doubleTap);
	if(inputComponents[ComponentSystemClick]){
		vr::VRDriverInput()->UpdateBooleanComponent(inputComponents[ComponentSystemClick], systemClick, 0);
	}
	if(inputComponents[ComponentTap]){
		vr::VRDriverInput()->UpdateBooleanComponent(inputComponents[ComponentTap], doubleTap, 0);
	}

	// Update the battery level (Crystal OG).
	const vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);
	const int batteryPercentage = pvr_getTrackedDeviceIntProperty(
		GetPvrSession(), pvrTrackedDevice_HMD, pvrTrackedDeviceProp_BatteryPercent_int, -1);
	if (batteryPercentage > 0) {
		vr::VRProperties()->SetFloatProperty(container, vr::Prop_DeviceBatteryPercentage_Float, batteryPercentage / 100.f);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
	}

	PollMagicAttach();

	if (s_cameraDriver) {
		s_cameraDriver->RunFrame();
	}
}

void PimaxSlamDriver::HandleEvent(const vr::VREvent_t& event) {
	switch (event.eventType) {
	case vr::VREvent_Input_HapticVibration:
		// CustomHeadsetDeviceProvider invokes this call back from the same context as RunFrame(). No need to acquire s_controllerDriverMutex.
		for (uint32_t side = 0; side < 2; side++) {
			if (s_controllerDriver[side]) {
				s_controllerDriver[side]->BroadcastHapticEvent(event.data.hapticVibration);
			}
		}
		break;
	case vr::VREvent_SceneApplicationChanged:
		SetSceneApplicationProcess(event.data.process.pid);
		break;
	}
}

void PimaxSlamDriver::StartPvrTracking() {
	if (GetPvrSession() && !pvrTrackingRunning.exchange(true)) {
		// Doing an auto-recenter is not ideal, but it's better than PVR spawning us in the middle of nowhere.
		// Players tend to start from a similar location at every boot.
		pvr_setTrackingOriginType(GetPvrSession(), pvrTrackingOrigin_FloorLevel);
		pvr_recenterTrackingOrigin(GetPvrSession());

		DriverLog("Starting PVR tracking thread");
		pvrTrackingThread = std::thread(&PimaxSlamDriver::PvrTrackingThread, this);
	}
}

void PimaxSlamDriver::StopPvrTracking() {
	if (pvrTrackingRunning.exchange(false) && pvrTrackingThread.joinable()) {
		DriverLog("Stopping PVR tracking thread");
		pvrTrackingThread.join();
		pvrTrackingThread = {};
	}
}

void PimaxSlamDriver::PvrTrackingThread() {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	const HANDLE timer = CreateWaitableTimer(nullptr, false, nullptr);
	const LARGE_INTEGER noDelay = {};
	// TODO: Make sure this is a reasonable value. 500Hz is quite high, but it doesn't seem to largely increase CPU/GPU utilization.
	const LONG periodMs = 2;
	SetWaitableTimer(timer, &noDelay, periodMs, nullptr, nullptr, true);

	while (true) {
		WaitForSingleObject(timer, 100);
		if (!pvrTrackingRunning) {
			break;
		}

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

		{
			std::shared_lock lock(s_controllerDriverMutex);

			for (uint32_t side = 0; side < 2; side++) {
				if (s_controllerDriver[side]) {
					s_controllerDriver[side]->UpdateTrackingState(trackingState.HandPoses[side]);
				}
			}
		}
	}

	CloseHandle(timer);

	DriverLog("PVR tracking thread stopped");
}
