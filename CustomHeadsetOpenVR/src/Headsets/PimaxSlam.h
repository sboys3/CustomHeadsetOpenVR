#pragma once
#include "BaseHeadset.h"
#include "../Helpers/PimaxCommon.h"
#include <atomic>
#include <thread>

class PimaxSlamDriver : public BaseHeadsetShim, public PimaxCommon {
public:
	virtual bool IsDesiredHeadset(std::string model, vr::PropertyContainerHandle_t container) override;

	virtual Config::BaseHeadsetConfig& GetConfig() override;
	virtual Config::BaseHeadsetConfig& GetConfigOld() override;

	virtual void PosTrackedDeviceActivate(uint32_t& unObjectId, vr::EVRInitError& returnValue) override;
	virtual void SubDeactivate() override;
	virtual void RunFrame() override;
	virtual void HandleEvent(const vr::VREvent_t& event) override;

	void StartPvrTracking();
	void StopPvrTracking();

private:
	enum InputComponents {
		ComponentSystemClick,
		ComponentTap,
		ComponentPresence,

		ComponentCount,
	};

	void PvrTrackingThread();

	std::thread pvrTrackingThread;
	std::atomic<bool> pvrTrackingRunning = false;
	vr::VRInputComponentHandle_t inputComponents[ComponentCount] = {};
};
