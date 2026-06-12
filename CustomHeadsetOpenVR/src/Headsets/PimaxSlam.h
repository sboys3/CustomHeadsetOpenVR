#pragma once
#include "BaseHeadset.h"
// TODO(mbucchia): Migrate eye tracking to PimaxCommon
#include "../Helpers/PimaxEyeTrackingBridge.h"
#include "../Helpers/PimaxCommon.h"

class PimaxSlamDriver : public BaseHeadsetShim, public PimaxCommon {
public:
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;

	virtual Config::BaseHeadsetConfig& GetConfig() override;
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;

	virtual void PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue) override;
	virtual void SubDeactivate() override;
	virtual void SubRunFrame() override;
	virtual void HandleEvent(const vr::VREvent_t& event) override;

private:
	PimaxEyeTrackingBridge eyeTracking;
};
