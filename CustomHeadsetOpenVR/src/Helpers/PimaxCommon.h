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
	float cantingAngle = 0;
	float ipd = 0;
	// if the headset is directly connected instead of using the PVR API
	bool directConnected = false;
	int directButtonState = 0;
	std::string headsetName;
};

class PimaxCommon {
public:
	PimaxCommon();
	virtual ~PimaxCommon() = default;
	static PimaxInfo GetInfo();
	static bool TryDirectConnection();
	static bool IsPimaxLighthouseDevice(std::string_view model, std::string_view manufacturer);
	static bool IsLighthouseHeadsetConnected();
	static bool IsSlamHeadsetConnected();
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

	void SetVisibilityMeshes();

	void PollMagicAttach();
	void SetSceneApplicationProcess(uint32_t pid);

	void GetHmdButtonsState(bool& systemButton, bool& doubleTap);

private:
	void EyeTrackingThread();

	pvrHmdInfo hmdInfo = {};
	bool hasEyeTracking = false;
	std::thread eyeTrackingThread;
	std::atomic<bool> eyeTrackingRunning;
	uint32_t lastSceneApplicationPid = 0;
	bool isLibMagicEnabled = false;
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
