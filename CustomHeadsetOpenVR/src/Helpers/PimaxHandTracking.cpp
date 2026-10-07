#include "PimaxHandTracking.h"
#include "../Driver/DriverLog.h"

#include "PimaxCommon.h"

#ifdef PVR_EXISTS

#include "MiscHelper.h"
#include "openvr_driver.h"

#include <DirectXMath.h>
#include <memory>
#include <shared_mutex>

using namespace DirectX;

// Matches driver_aapvr.
static constexpr uint64_t k_UniverseId = 30;

// The mapping from the OpenVR hand skeleton (31 bones) to the PVR hand skeleton (32 bones).
// The PVR root bone (0) is reported with garbage data, so it is not used.
// The PVR palm bone (26) has no OpenVR counterpart, so the auxiliary bones are offset by one.
static const uint32_t openvrToPvrBone[31] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 27, 28, 29, 30, 31
};

// The parent bone of each OpenVR bone, converted to a PVR bone index.
// The OpenVR root and wrist bones are aligned with the device pose, so both resolve to the PVR wrist bone.
static const uint32_t pvrParentBone[31] = {
	1, 1, 1, 2, 3, 4, 1, 6, 7, 8, 9, 1, 11, 12, 13, 14,
	1, 16, 17, 18, 19, 1, 21, 22, 23, 24, 5, 10, 15, 20, 25
};

static void FillIdentityTransforms(vr::VRBoneTransform_t* outTransforms) {
	for (uint32_t bone = 0; bone < 31; bone++) {
		outTransforms[bone] = {};
		outTransforms[bone].position.v[3] = 1.0f;
		outTransforms[bone].orientation.w = 1.0f;
	}
}

static vr::VRBoneTransform_t MakeBoneTransform(const pvrPosef& parent, const pvrPosef& bone) {
	DirectX::XMMATRIX parentMatrix = XMMatrixTranslation(parent.Position.x, parent.Position.y, parent.Position.z) *
		XMMatrixRotationQuaternion(DirectX::XMVectorSet(parent.Orientation.x, parent.Orientation.y, parent.Orientation.z, parent.Orientation.w));
	DirectX::XMMATRIX boneMatrix = XMMatrixTranslation(bone.Position.x, bone.Position.y, bone.Position.z) *
		XMMatrixRotationQuaternion(DirectX::XMVectorSet(bone.Orientation.x, bone.Orientation.y, bone.Orientation.z, bone.Orientation.w));

	// SteamVR expects the bone transforms to be relative to the parent bone.
	DirectX::XMMATRIX localMatrix = XMMatrixMultiply(XMMatrixInverse(nullptr, parentMatrix), boneMatrix);

	DirectX::XMVECTOR position, orientation, scale;
	DirectX::XMMatrixDecompose(&scale, &orientation, &position, localMatrix);
	
	vr::VRBoneTransform_t transform = {};
	transform.position.v[0] = DirectX::XMVectorGetX(position);
	transform.position.v[1] = DirectX::XMVectorGetY(position);
	transform.position.v[2] = DirectX::XMVectorGetZ(position);
	transform.position.v[3] = 1.0f;
	transform.orientation.x = DirectX::XMVectorGetX(orientation);
	transform.orientation.y = DirectX::XMVectorGetY(orientation);
	transform.orientation.z = DirectX::XMVectorGetZ(orientation);
	transform.orientation.w = DirectX::XMVectorGetW(orientation);
	return transform;
}

static void ComputeBoneTransforms(const pvrSkeletalData& skeletalData, vr::VRBoneTransform_t* outTransforms) {
	FillIdentityTransforms(outTransforms);
	for (uint32_t bone = 2; bone < 31; bone++) {
		outTransforms[bone] = MakeBoneTransform(
			skeletalData.boneTransforms[pvrParentBone[bone]],
			skeletalData.boneTransforms[openvrToPvrBone[bone]]);
	}
}

// A driver for a hand that is tracked by a Pimax headset.
class PimaxHandDriver : public vr::ITrackedDeviceServerDriver {
public:
	PimaxHandDriver(vr::ETrackedControllerRole role) : role(role), side(role == vr::TrackedControllerRole_LeftHand ? 0 : 1) {
	}

	vr::EVRInitError Activate(uint32_t unObjectId) override {
		deviceIndex = unObjectId;

		const bool isLeft = role == vr::TrackedControllerRole_LeftHand;

		const vr::PropertyContainerHandle_t container =
			vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);

		vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, role);

		vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "aapvr");
		vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "Pimax");
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_ModelNumber_String, isLeft ? "Pimax Hand (Left)" : "Pimax Hand (Right)");
		if(k_UniverseId){
			vr::VRProperties()->SetUint64Property(container, vr::Prop_CurrentUniverseId_Uint64, k_UniverseId);
		}
		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_RenderModelName_String, "{vrlink}/rendermodels/shuttlecock");
		vr::VRProperties()->SetStringProperty(container, vr::Prop_ControllerType_String, "svl_hand_interaction_augmented");

		vr::VRProperties()->SetStringProperty(
			container, vr::Prop_InputProfilePath_String, "{vrlink}/input/svl_hand_interaction_augmented_input_profile.json");

		// Deprioritize these devices so that real controllers are selected for the hand assignment.
		vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerHandSelectionPriority_Int32, -1);

		// Create the skeleton component.
		vr::EVRInputError inputError = vr::VRDriverInput()->CreateSkeletonComponent(
			container,
			isLeft ? "/input/skeleton/left" : "/input/skeleton/right",
			isLeft ? "/skeleton/hand/left" : "/skeleton/hand/right",
			"/pose/raw",
			vr::VRSkeletalTracking_Full,
			nullptr, 0, &skeletonComponentHandle);
		if (inputError != vr::VRInputError_None) {
			DriverLog("PimaxHandDriver: Failed to create the skeleton component, error: %d", inputError);
		}

		// SteamVR wants skeletal data immediately.
		vr::VRBoneTransform_t identityTransforms[31];
		FillIdentityTransforms(identityTransforms);
		UpdateSkeleton(identityTransforms);
		UpdateSkeleton(identityTransforms);

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

	void UpdateTrackingState(bool enabled, const pvrSkeletalData& skeletalData, const pvrHandTrackingInputState& inputState) {
		if (deviceIndex == vr::k_unTrackedDeviceIndexInvalid) {
			return;
		}

		// The fetch can return a success code with zero bones, so the bone count is also checked.
		const bool isValid = enabled &&
			skeletalData.boneCount >= PVR_MAX_SKELETAL_BONE_COUNT &&
			inputState.IsValid[side];

		vr::DriverPose_t pose = {};
		pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
		pose.deviceIsConnected = isValid;
		pose.result = vr::TrackingResult_Running_OutOfRange;

		if (isValid) {
			// The hand bones are reported in the same tracking space as the headset,
			// so the wrist pose can be used directly as the device pose.
			const pvrPosef& wrist = skeletalData.boneTransforms[1];
			pose.vecPosition[0] = wrist.Position.x;
			pose.vecPosition[1] = wrist.Position.y;
			pose.vecPosition[2] = wrist.Position.z;
			pose.qRotation.x = wrist.Orientation.x;
			pose.qRotation.y = wrist.Orientation.y;
			pose.qRotation.z = wrist.Orientation.z;
			pose.qRotation.w = wrist.Orientation.w;

			pose.poseIsValid = true;
			pose.result = vr::TrackingResult_Running_OK;

			vr::VRBoneTransform_t transforms[31];
			ComputeBoneTransforms(skeletalData, transforms);
			UpdateSkeleton(transforms);
		}

		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(pose));
	}

	void Disconnect() {
		vr::DriverPose_t pose = {};
		pose.qWorldFromDriverRotation.w = pose.qDriverFromHeadRotation.w = pose.qRotation.w = 1.0;
		pose.result = vr::TrackingResult_Running_OutOfRange;
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(pose));
	}

private:
	void UpdateSkeleton(const vr::VRBoneTransform_t* transforms) {
		if (skeletonComponentHandle == vr::k_ulInvalidInputComponentHandle) {
			return;
		}

		// The data needs to be updated for both motion ranges.
		const vr::EVRInputError updateErrorWithController = vr::VRDriverInput()->UpdateSkeletonComponent(skeletonComponentHandle, vr::VRSkeletalMotionRange_WithController, transforms, 31);
		const vr::EVRInputError updateErrorWithoutController = vr::VRDriverInput()->UpdateSkeletonComponent(skeletonComponentHandle, vr::VRSkeletalMotionRange_WithoutController, transforms, 31);
		if ((updateErrorWithController != vr::VRInputError_None || updateErrorWithoutController != vr::VRInputError_None) &&
			!loggedUpdateFailure) {
			loggedUpdateFailure = true;
			DriverLog("PimaxHandDriver: UpdateSkeletonComponent failed, with controller: %d, without controller: %d",
				updateErrorWithController, updateErrorWithoutController);
		}
	}

	const vr::ETrackedControllerRole role;
	const uint32_t side;
	vr::TrackedDeviceIndex_t deviceIndex = vr::k_unTrackedDeviceIndexInvalid;
	vr::VRInputComponentHandle_t skeletonComponentHandle = vr::k_ulInvalidInputComponentHandle;
	bool loggedUpdateFailure = false;
};

static std::unique_ptr<PimaxHandDriver> s_handDriver[2];
static std::shared_mutex s_handDriverMutex;

PimaxHandTracking::PimaxHandTracking(){
}

PimaxHandTracking::~PimaxHandTracking(){
}

void PimaxHandTracking::Stop(){
	DriverLog("PimaxHandTracking: Stopping hand tracking");
	{
		// Protects s_handDriver[].
		std::unique_lock lock(s_handDriverMutex);
		for (uint32_t side = 0; side < 2; side++) {
			if (s_handDriver[side]) {
				s_handDriver[side]->Disconnect();
			}
		}
	}
}

void PimaxHandTracking::RunBackground(){
	bool enabled = PimaxCommon::GetHeadsetConfig().enablePimaxHandTracking;
	if(!enabled){
		return;
	}
	if(!probed){
		shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
		const pvrSessionHandle session = PimaxCommon::GetPvrSession();
		// Only probe once the session is fully established so that a missed probe is retried.
		if(session && session->envh && session->envh->pvr_interface){
			pvrHandApiAvailable = session->envh->pvr_interface->getHandTrackingSkeletalData != nullptr &&
				session->envh->pvr_interface->getHandTrackingInputState != nullptr;
			supported = pvrHandApiAvailable;
			DriverLog("PimaxHandTracking: %s",
				pvrHandApiAvailable ? "hand tracking is supported by the PVR SDK" : "hand tracking is not supported by the PVR SDK");
			probed = true;
		}
	}
	if(!supported){
		return;
	}
	
	// Manage the hand tracking devices.
	for (uint32_t side = 0; side < 2; side++) {
		// Protects creation of s_handDriver[] and updating the device poses inside PimaxHandDriver.
		std::unique_lock lock(s_handDriverMutex);
		if(enabled){
			if(!s_handDriver[side]){
				const pvrSessionHandle session = PimaxCommon::GetPvrSession();
				pvrHandTrackingInputState inputState = {};
				pvr_getHandTrackingInputState(session, &inputState);
				if(!inputState.IsValid[side]){
					continue;
				}
				s_handDriver[side] = std::make_unique<PimaxHandDriver>(
					side == 0 ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);
				vr::VRServerDriverHost()->TrackedDeviceAdded(
					side == 0 ? "PIMAXHANDLEFT" : "PIMAXHANDRIGHT", vr::TrackedDeviceClass_Controller, s_handDriver[side].get());
				DriverLog("PimaxHandTracking: Added hand tracking device for the %s hand", side == 0 ? "left" : "right");
			}
		}
		else if(s_handDriver[side]){
			s_handDriver[side]->Disconnect();
		}
	}
}

void PimaxHandTracking::RunFrame(){
	if(!supported){
		return;
	}
	
	bool enabled = PimaxCommon::GetHeadsetConfig().enablePimaxHandTracking;
	
	pvrSkeletalData skeletalData[2] = {};
	pvrHandTrackingInputState inputState = {};
	{
		shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
		const pvrSessionHandle session = PimaxCommon::GetPvrSession();
		// The function pointers are not present in older PVR SDKs, so only call them when they are available.
		if(session && session->envh && pvrHandApiAvailable){
			const auto pvrNow = PimaxCommon::GetPvrTime();
			for (uint32_t side = 0; side < 2; side++) {
				pvr_getHandTrackingSkeletalData(session, (pvrHandDeviceType)side, pvrNow, &skeletalData[side]);
			}
			pvr_getHandTrackingInputState(session, &inputState);
		}
	}
	
	{
		std::shared_lock lock(s_handDriverMutex);
		for (uint32_t side = 0; side < 2; side++) {
			if (s_handDriver[side]) {
				s_handDriver[side]->UpdateTrackingState(enabled, skeletalData[side], inputState);
			}
		}
	}
}

#else

PimaxHandTracking::PimaxHandTracking(){
}

PimaxHandTracking::~PimaxHandTracking(){
}

void PimaxHandTracking::Stop(){
}

void PimaxHandTracking::RunBackground(){
}

void PimaxHandTracking::RunFrame(){
}

#endif // PVR_EXISTS

PimaxHandTracking pimaxHandTracking = {};
