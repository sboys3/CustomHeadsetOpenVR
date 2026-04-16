#pragma once
#include "BaseHeadset.h"

class DreamAirShim : public BaseHeadsetShim{
public:
	// function that returns if this is a MeganeX8K headset
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;
	// function that returns the config for this headset
	virtual Config::BaseHeadsetConfig& GetConfig() override;
	// function that returns the old config for this headset
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;
};