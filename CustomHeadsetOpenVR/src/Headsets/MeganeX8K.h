#pragma once
#include "BaseHeadset.h"

class MeganeX8KShim : public BaseHeadsetShim{
public:
	// function that returns if this is a MeganeX8K headset
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;
	// function that returns the config for this headset
	virtual Config::BaseHeadsetConfig& GetConfig() override;
	// function that returns the old config for this headset
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;
	// revision of the headset 1: original 2: mk2
	int headsetRevision = 1;
};