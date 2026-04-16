#pragma once
#include "BaseHeadset.h"

class FakeHeadset : public BaseHeadsetShim{
public:
	// function that returns if this is a MeganeX8K headset
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;
	// function that returns the config for this headset
	virtual Config::BaseHeadsetConfig& GetConfig() override;
	// function that returns the old config for this headset
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;
	
	// when using BaseHeadsetShim, use pos functions for device functions and pre functions for display functions
	// functions should call their parent method unless completely replacing functionality from BaseHeadsetShim
	
	// override the activation function
	virtual void PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue) override;
	// run every frame
	virtual void RunFrame() override;
};