#pragma once
#include "BaseHeadset.h"
#include "../Helpers/PimaxCommon.h"

class PimaxLighthouseShim : public BaseHeadsetShim, public PimaxCommon{
public:
	// function that returns if this is a Pimax headset
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;
	// function that returns the config for this headset
	virtual Config::BaseHeadsetConfig& GetConfig() override;
	// function that returns the old config for this headset
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;
	// set pvr config, start eye tracking
	virtual void PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue) override;
	// stop eye tracking
	virtual void SubDeactivate() override;
	// run eye tracking
	virtual void RunFrame() override;
	// run pvr background tasks
	virtual void RunPvrBackground() override;
	// forward events
	virtual void HandleEvent(const vr::VREvent_t& event) override;

private:
};
