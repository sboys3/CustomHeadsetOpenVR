#pragma once

#include <array>
#include <string>

#include "openvr_driver.h"
#include <atomic>
#include <thread>


/**
 * This class implements overrides for functions in SteamVR devices
 * Pre functions run before the original function and can return false to prevent the original function from running or modify the parameters that will be passed to the original function.
 * Post functions run after the original function and can modify the return value or other parameters.
 * 
 * The shimActive flag can be used to disable the shim at runtime. 
 * 
 * The trackedDevice and displayComponent pointers is the original tracked device that the shim is wrapping. 
 */
class ShimDefinition{
public:
	bool shimActive = true;
	
	// original tracked device
	vr::ITrackedDeviceServerDriver *trackedDevice = NULL;
	virtual bool PreTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){return true;};
	virtual void PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){};
	virtual bool PreTrackedDeviceDeactivate(){return true;};
	virtual void PosTrackedDeviceDeactivate(){};
	virtual bool PreTrackedDeviceEnterStandby(){return true;};
	virtual void PosTrackedDeviceEnterStandby(){};
	virtual bool PreTrackedDeviceDebugRequest(const char *&pchRequest, char *&pchResponseBuffer, uint32_t &unResponseBufferSize){return true;};
	virtual void PosTrackedDeviceDebugRequest(const char *&pchRequest, char *&pchResponseBuffer, uint32_t &unResponseBufferSize){};
	virtual bool PreTrackedDeviceGetPose(vr::DriverPose_t &returnPose){return true;};
	virtual void PosTrackedDeviceGetPose(vr::DriverPose_t &returnPose){};
	virtual bool PreTrackedDeviceGetComponent(const char *&pchComponentNameAndVersion, void *&returnComponent){return true;};
	virtual void PosTrackedDeviceGetComponent(const char *&pchComponentNameAndVersion, void *&returnComponent){};
	
	// original display component
	vr::IVRDisplayComponent *displayComponent = NULL;
	virtual bool PreDisplayComponentIsDisplayOnDesktop(bool &returnValue){return true;};
	virtual void PosDisplayComponentIsDisplayOnDesktop(bool &returnValue){};
	virtual bool PreDisplayComponentIsDisplayRealDisplay(bool &returnValue){return true;};
	virtual void PosDisplayComponentIsDisplayRealDisplay(bool &returnValue){};
	virtual bool PreDisplayComponentGetRecommendedRenderTargetSize(uint32_t *&pnWidth, uint32_t *&pnHeight){return true;};
	virtual void PosDisplayComponentGetRecommendedRenderTargetSize(uint32_t *&pnWidth, uint32_t *&pnHeight){};
	virtual bool PreDisplayComponentGetEyeOutputViewport(vr::EVREye &eEye, uint32_t *&pnX, uint32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){return true;};
	virtual void PosDisplayComponentGetEyeOutputViewport(vr::EVREye &eEye, uint32_t *&pnX, uint32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){};
	virtual bool PreDisplayComponentGetProjectionRaw(vr::EVREye &eEye, float *&pfLeft, float *&pfRight, float *&pfTop, float *&pfBottom){return true;};
	virtual void PosDisplayComponentGetProjectionRaw(vr::EVREye &eEye, float *&pfLeft, float *&pfRight, float *&pfTop, float *&pfBottom){};
	virtual bool PreDisplayComponentComputeDistortion(vr::EVREye &eEye, float &fU, float &fV, vr::DistortionCoordinates_t &returnValue){return true;};
	virtual void PosDisplayComponentComputeDistortion(vr::EVREye &eEye, float &fU, float &fV, vr::DistortionCoordinates_t &returnValue){};
	virtual bool PreDisplayComponentComputeInverseDistortion(vr::HmdVector2_t *&pResult, vr::EVREye &eEye, uint32_t &unChannel, float &fU, float &fV, bool &returnValue){return true;};
	virtual void PosDisplayComponentComputeInverseDistortion(vr::HmdVector2_t *&pResult, vr::EVREye &eEye, uint32_t &unChannel, float &fU, float &fV, bool &returnValue){};
	virtual bool PreDisplayComponentGetWindowBounds(int32_t *&pnX, int32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){return true;};
	virtual void PosDisplayComponentGetWindowBounds(int32_t *&pnX, int32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){};
	
	// non-shim functions
	
	// run on every frame of the main loop of the server
	virtual void RunFrame(){};
};

class ShimTrackedDeviceDriver : public vr::ITrackedDeviceServerDriver{
public:
	ShimTrackedDeviceDriver(ShimDefinition* shimDefinition, vr::ITrackedDeviceServerDriver* original);
	ShimDefinition* shimDefinition;
	
	vr::EVRInitError Activate( uint32_t unObjectId ) override;
	void EnterStandby() override;
	void *GetComponent( const char *pchComponentNameAndVersion ) override;
	void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize ) override;
	vr::DriverPose_t GetPose() override;
	void Deactivate() override;
};

class ShimDisplayComponent : public vr::IVRDisplayComponent{
public:
	explicit ShimDisplayComponent(ShimDefinition* shimDefinition, vr::IVRDisplayComponent *original);
	ShimDefinition* shimDefinition;
	
	bool IsDisplayOnDesktop() override;
	bool IsDisplayRealDisplay() override;
	void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight) override;
	void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override;
	void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom) override;
	vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV) override;
	bool ComputeInverseDistortion(vr::HmdVector2_t* pResult, vr::EVREye eEye, uint32_t unChannel, float fU, float fV) override;
	void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override;
};

