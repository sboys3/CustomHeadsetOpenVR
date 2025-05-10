#include "../Driver/DeviceShim.h"
#include "../Driver/DeviceProvider.h"
#include "../Driver/DriverLog.h"

#include "../Distortion/DistortionProfile.h"

#include <thread>
#include <cmath>


class MeganeX8KShim : public ShimDefinition{
public:
	CustomHeadsetDeviceProvider* deviceProvider;
	
	DistortionProfile* distortionProfile;
	
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
	
	void SetIPD(float ipd);
	
	virtual void RunFrame() override;
	
	void UpdateSettings();
	
	void TestThread();
};