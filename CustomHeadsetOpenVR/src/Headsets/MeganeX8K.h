#include "../Driver/DeviceShim.h"
#include "../Driver/DeviceProvider.h"
#include "../Driver/DriverLog.h"

#include "../Distortion/DistortionProfileConstructor.h"

#include <thread>
#include <cmath>
#include <mutex>


class MeganeX8KShim : public ShimDefinition{
public:
	CustomHeadsetDeviceProvider* deviceProvider = nullptr;
	
	DistortionProfileConstructor distortionProfileConstructor;
	std::mutex distortionProfileLock;
	// time that last frame occurred
	double lastFrameTime = 0;
	// last time the distortion was changed
	double lastDistortionChangeTime = 0;
	// the distortion profile may be changed in the middle of reading, so do a final update
	bool needsDistortionFinalization = true;
	// the offset used for burn in prevention
	float fovBurnInOffset = 0;
	// last color multiplier applied
	Config::Color lastColorMultiplier = {};
	// last time the the headset was detected as moving
	double lastMovementTime = 0;
	// last rotation of the headset that was detected as movement
	vr::HmdMatrix34_t lastMovementRotation = {};
	// brightness multiplier from for dimming when the headset is not moving
	double dimmingMultiplier = 1;
	
	
	// test thread that toggles testToggle every 5 seconds to test things
	bool testToggle = false;
	bool isActive = false;
	std::thread testThread;
	
	virtual void PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue) override;
	virtual void PosTrackedDeviceDeactivate() override;
	virtual bool PreDisplayComponentGetProjectionRaw(vr::EVREye &eEye, float *&pfLeft, float *&pfRight, float *&pfBottom, float *&pfTop) override;
	virtual bool PreDisplayComponentComputeDistortion(vr::EVREye &eEye, float &fU, float &fV, vr::DistortionCoordinates_t &coordinates) override;
	virtual bool PreDisplayComponentIsDisplayOnDesktop(bool &returnValue) override;
	virtual bool PreDisplayComponentIsDisplayRealDisplay(bool &returnValue) override;
	virtual bool PreDisplayComponentGetWindowBounds(int32_t *&pnX, int32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight) override;
	virtual bool PreDisplayComponentGetEyeOutputViewport(vr::EVREye &eEye, uint32_t *&pnX, uint32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight) override;
	virtual bool PreDisplayComponentGetRecommendedRenderTargetSize(uint32_t* &pnWidth, uint32_t* &pnHeight) override;
	
	// helper function to calculate the 100% render resolution
	void GetRecommendedRenderTargetSize(uint32_t* renderWidth, uint32_t* renderHeight);
	
	void SetIPD(float ipd, float angle = 0);
	
	virtual void RunFrame() override;
	
	void UpdateSettings();
	
	void TestThread();
};