#pragma once
#include "BaseHeadset.h"
// TODO(mbucchia): Migrate eye tracking to PimaxCommon
#include "../Helpers/PimaxEyeTrackingBridge.h"
#include "../Helpers/PimaxCommon.h"

class PimaxLighthouseShim : public BaseHeadsetShim, public PimaxCommon{
public:
	// function that returns if this is a Pimax headset
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;
	// function that returns the config for this headset
	virtual Config::BaseHeadsetConfig& GetConfig() override;
	// function that returns the old config for this headset
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;
	// eye tracking
	PimaxEyeTrackingBridge eyeTracking;
	// start eye tracking
	virtual void SubActivate(vr::PropertyContainerHandle_t container) override;
	// stop eye tracking
	virtual void SubDeactivate() override;
	// run eye tracking
	virtual void SubRunFrame() override;
};
