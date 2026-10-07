#include "PimaxCamera.h"

#ifdef PVR_EXISTS

#include "../Driver/DriverLog.h"
#include "../Helpers/PimaxCommon.h"
#include "../Helpers/MiscHelper.h"
#include "../Helpers/vr_blockqueue.h"

#include <DirectXMath.h>
#include <memory>
#include <PVR_Math.h>
#include <shared_mutex>

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

PimaxCamera::PimaxCamera() {
	QueryPerformanceFrequency(&qpcFrequency);
}

std::unique_ptr<PimaxCamera> PimaxCamera::TryCreatePassthrough() {
	bool isPimaxRuntimeNewEnough = false;
	std::string pimaxEvoVersion = GetInstalledProgramVersion("PimaxEVO");
	if(pimaxEvoVersion != ""){
		// Versions older than 83 will hang when attempting to get formats for the camera.
		isPimaxRuntimeNewEnough |= IsNewVersion("1.0.0.82", pimaxEvoVersion);
		DriverLog("Installed PimaxEVO version: %s, new enough for passthrough: %i", pimaxEvoVersion.c_str(), isPimaxRuntimeNewEnough);
	}
	std::string pimaxPlayVersion = GetInstalledProgramVersion("PimaxPlay");
	if(pimaxPlayVersion != ""){
		// I'm not sure where the cutoff is, but I verified that 135 works.
		isPimaxRuntimeNewEnough |= IsNewVersion("2.0.0.134", pimaxPlayVersion);
		DriverLog("Installed PimaxPlay version: %s, new enough for passthrough: %i", pimaxPlayVersion.c_str(), isPimaxRuntimeNewEnough);
	}
	if((isPimaxRuntimeNewEnough && PimaxCommon::GetHeadsetConfig().enablePimaxPassthrough) || PimaxCommon::GetHeadsetConfig().forcePimaxPassthrough){
		shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
		const auto vstType = pvr_getVSTType(PimaxCommon::GetPvrSession());
		if (vstType != pvrVSTTypeNone) {
			const auto format = pvr_getVSTStreamFormat(PimaxCommon::GetPvrSession());
			if (format == pvrVST_FORMAT_NV12 || format == pvrVST_FORMAT_RAW8) {
				return std::make_unique<PimaxCamera>();
			}
		}
	}
	return nullptr;
}

void PimaxCamera::Activate(vr::TrackedDeviceIndex_t deviceIndex) {
	const vr::PropertyContainerHandle_t container =
		vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);
		
	shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
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

bool PimaxCamera::GetCameraFrameDimensions(vr::ECameraVideoStreamFormat nVideoStreamFormat, uint32_t* pWidth, uint32_t* pHeight) {
	bool status = false;
	if (nVideoStreamFormat == vr::CVS_FORMAT_NV12 || nVideoStreamFormat == vr::CVS_FORMAT_UNKNOWN) {
		*pWidth = cameraResolutionWidth * numCameras;
		*pHeight = cameraResolutionHeight;
		status = true;
	}

	return status;
}

bool PimaxCamera::GetCameraFrameBufferingRequirements(int* pDefaultFrameQueueSize, uint32_t* pFrameBufferDataSize) {
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

bool PimaxCamera::SetCameraFrameBuffering(int nFrameBufferCount, void** ppFrameBuffers, uint32_t nFrameBufferDataSize) {
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

bool PimaxCamera::SetCameraVideoStreamFormat(vr::ECameraVideoStreamFormat nVideoStreamFormat) {
	bool status = false;
	if (nVideoStreamFormat == vr::CVS_FORMAT_NV12) {
		status = true;
	}

	return status;
}

vr::ECameraVideoStreamFormat PimaxCamera::GetCameraVideoStreamFormat() {
	return vr::CVS_FORMAT_NV12;
}

bool PimaxCamera::StartVideoStream() {
	QueryPerformanceCounter(&cameraStartTime);
	cameraFrameIndex = 0;
	cameraActive = true;
	cameraPaused = false;

	return true;
}

void PimaxCamera::StopVideoStream() {
	cameraActive = cameraPaused = false;
}

bool PimaxCamera::IsVideoStreamActive(bool* pbPaused, float* pflElapsedTime) {
	*pbPaused = cameraActive && cameraPaused;
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	*pflElapsedTime =
		cameraActive ? (now.QuadPart - cameraStartTime.QuadPart) / (float)qpcFrequency.QuadPart : 0.f;

	return cameraActive;
}

const vr::CameraVideoStreamFrame_t* PimaxCamera::GetVideoStreamFrame() {
	// Not supported.
	return nullptr;
}

void PimaxCamera::ReleaseVideoStreamFrame(const vr::CameraVideoStreamFrame_t* pFrameImage) {
}

bool PimaxCamera::SetAutoExposure(bool bEnable) {
	// Not supported.
	return false;
}

bool PimaxCamera::PauseVideoStream() {
	cameraPaused = true;
	return true;
}

bool PimaxCamera::ResumeVideoStream() {
	cameraPaused = false;
	return true;
}

bool PimaxCamera::GetCameraDistortion(uint32_t nCameraIndex, float flInputU, float flInputV, float* pflOutputU, float* pflOutputV) {
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

bool PimaxCamera::GetCameraProjection(uint32_t nCameraIndex, vr::EVRTrackedCameraFrameType eFrameType, float flZNear, float flZFar, vr::HmdMatrix44_t* pProjection) {
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

bool PimaxCamera::SetFrameRate(int nISPFrameRate, int nSensorFrameRate) {
	// We cannot service this request. Pretend success.
	return true;
}

bool PimaxCamera::SetCameraVideoSinkCallback(vr::ICameraVideoSinkCallback* pCameraVideoSinkCallback) {
	cameraSinkCallback = pCameraVideoSinkCallback;
	return true;
}

bool PimaxCamera::GetCameraCompatibilityMode(vr::ECameraCompatibilityMode* pCameraCompatibilityMode) {
	*pCameraCompatibilityMode = vr::CAMERA_COMPAT_MODE_ISO_30FPS;
	return true;
}

bool PimaxCamera::SetCameraCompatibilityMode(vr::ECameraCompatibilityMode nCameraCompatibilityMode) {
	bool status = false;
	if (nCameraCompatibilityMode == vr::CAMERA_COMPAT_MODE_ISO_30FPS) {
		status = true;
	}

	return status;
}

bool PimaxCamera::GetCameraFrameBounds(vr::EVRTrackedCameraFrameType eFrameType, uint32_t* pLeft, uint32_t* pTop, uint32_t* pWidth, uint32_t* pHeight) {
	*pLeft = *pTop = 0;
	*pWidth = cameraResolutionWidth * numCameras;
	*pHeight = cameraResolutionHeight;

	return true;
}

bool PimaxCamera::GetCameraIntrinsics(uint32_t nCameraIndex, vr::EVRTrackedCameraFrameType eFrameType, vr::HmdVector2_t* pFocalLength, vr::HmdVector2_t* pCenter, vr::EVRDistortionFunctionType* peDistortionType, double rCoefficients[vr::k_unMaxDistortionFunctionParameters]) {
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

void PimaxCamera::RunFrame() {
	if (cameraActive && !cameraPaused && cameraBuffer[cameraBufferIndex]) {
		// Flip buffer.
		cameraBufferIndex ^= 1;

		// Retrieve and convert the image from PVR.
		const uint32_t frameSize = (cameraResolutionWidth * cameraResolutionHeight * numCameras * 3) / 2;
		pvrVSTStreamFrame frame = {};
		shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
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

#endif // PVR_EXISTS
