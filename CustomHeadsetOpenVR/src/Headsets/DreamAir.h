#pragma once
#include "BaseHeadset.h"
#include "../Helpers/PimaxEyeTrackingBridge.h"

class DreamAirShim : public BaseHeadsetShim{
public:
	// function that returns if this is a MeganeX8K headset
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