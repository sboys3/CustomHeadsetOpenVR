#pragma once
#include "../Config/Config.h"

#include <PVR.h>
#include <PVR_API.h>
#include <atomic>
#include <string>
#include <thread>


struct PimaxInfo {
	bool connected = false;
	Config::HeadsetType headsetType = Config::HeadsetType::None;
	bool useSlamTracking = false;
	uint32_t resolutionX = 0;
	uint32_t resolutionY = 0;
};

class PimaxCommon {
public:
	PimaxCommon();
	virtual ~PimaxCommon() = default;
	static PimaxInfo GetInfo();
	static pvrSessionHandle GetPvrSession();
	static double GetPvrTime();
	static Config::BaseHeadsetConfig& PatchConfig(Config::BaseHeadsetConfig& config);
	static Config::BaseHeadsetConfig& GetHeadsetConfig();
	static Config::BaseHeadsetConfig& GetHeadsetConfigOld();

protected:
	pvrHmdInfo GetHmdInfo() const { return hmdInfo; };
	bool HasEyeTracking() const { return hasEyeTracking; }


	bool CheckDeviceLost();

	void StartEyeTracking();
	void StopEyeTracking();

private:
	void EyeTrackingThread();

	pvrHmdInfo hmdInfo = {};
	bool hasEyeTracking = false;
	std::thread eyeTrackingThread;
	std::atomic<bool> eyeTrackingRunning;
};

static inline std::string pvr_getTrackedDeviceStringPropertyHelper(pvrSessionHandle sessionHandle,
	pvrTrackedDeviceType device,
	pvrTrackedDeviceProp prop) {
	const int size = pvr_getTrackedDeviceStringProperty(sessionHandle, device, prop, nullptr, 0);
	if (size > 0) {
		std::string value(size, 0);
		pvr_getTrackedDeviceStringProperty(sessionHandle, device, prop, value.data(), (int)value.size() + 1);
		// Remove trailing 0.
		value.resize(size - 1, 0);
		return value;
	}
	return {};
}
