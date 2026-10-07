#pragma once
#include "../Config/Config.h"

#include "PimaxCommon.h"

#include "openvr_driver.h"
#include <atomic>
#include <memory>

#ifdef PVR_EXISTS
// A driver for Pimax Video See Through (VST).
// IVRCameraComponent is not officially documented.
// See https://github.com/Rectus/openvr_camera_sim/blob/main/camera_component.cpp
class PimaxCamera : public vr::IVRCameraComponent {
public:
	PimaxCamera();

	// Creates the camera driver if the Pimax runtime version and configuration allow camera passthrough.
	// Setting up the camera component must be done early, prior to any GetComponent().
	static std::unique_ptr<PimaxCamera> TryCreatePassthrough();

	void Activate(vr::TrackedDeviceIndex_t deviceIndex);
	void RunFrame();

	bool GetCameraFrameDimensions(vr::ECameraVideoStreamFormat nVideoStreamFormat, uint32_t* pWidth, uint32_t* pHeight) override;
	bool GetCameraFrameBufferingRequirements(int* pDefaultFrameQueueSize, uint32_t* pFrameBufferDataSize) override;
	bool SetCameraFrameBuffering(int nFrameBufferCount, void** ppFrameBuffers, uint32_t nFrameBufferDataSize) override;
	bool SetCameraVideoStreamFormat(vr::ECameraVideoStreamFormat nVideoStreamFormat) override;
	vr::ECameraVideoStreamFormat GetCameraVideoStreamFormat() override;
	bool StartVideoStream() override;
	void StopVideoStream() override;
	bool IsVideoStreamActive(bool* pbPaused, float* pflElapsedTime) override;
	const vr::CameraVideoStreamFrame_t* GetVideoStreamFrame() override;
	void ReleaseVideoStreamFrame(const vr::CameraVideoStreamFrame_t* pFrameImage) override;
	bool SetAutoExposure(bool bEnable) override;
	bool PauseVideoStream() override;
	bool ResumeVideoStream() override;
	bool GetCameraDistortion(uint32_t nCameraIndex, float flInputU, float flInputV, float* pflOutputU, float* pflOutputV) override;
	bool GetCameraProjection(uint32_t nCameraIndex, vr::EVRTrackedCameraFrameType eFrameType, float flZNear, float flZFar, vr::HmdMatrix44_t* pProjection) override;
	bool SetFrameRate(int nISPFrameRate, int nSensorFrameRate) override;
	bool SetCameraVideoSinkCallback(vr::ICameraVideoSinkCallback* pCameraVideoSinkCallback) override;
	bool GetCameraCompatibilityMode(vr::ECameraCompatibilityMode* pCameraCompatibilityMode) override;
	bool SetCameraCompatibilityMode(vr::ECameraCompatibilityMode nCameraCompatibilityMode) override;
	bool GetCameraFrameBounds(vr::EVRTrackedCameraFrameType eFrameType, uint32_t* pLeft, uint32_t* pTop, uint32_t* pWidth, uint32_t* pHeight) override;
	bool GetCameraIntrinsics(uint32_t nCameraIndex, vr::EVRTrackedCameraFrameType eFrameType, vr::HmdVector2_t* pFocalLength, vr::HmdVector2_t* pCenter, vr::EVRDistortionFunctionType* peDistortionType, double rCoefficients[vr::k_unMaxDistortionFunctionParameters]) override;

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
#endif // PVR_EXISTS
