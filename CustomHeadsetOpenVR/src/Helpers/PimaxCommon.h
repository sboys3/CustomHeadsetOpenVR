#pragma once
#include "../Config/Config.h"

#include <atomic>
#include <string>
#include <thread>
#include <shared_mutex>

// PVR SDK is Windows-only
#if __has_include("PVR.h")
#ifdef _WIN32
	#include <PVR.h>
	#include <PVR_API.h>
	#ifndef PVR_EXISTS
		#define PVR_EXISTS
	#endif
#endif
#endif

struct PimaxInfo {
	bool connected = false;
	bool pvrConnected = false;
	bool isOpenPortEnabled = false;
	Config::HeadsetType headsetType = Config::HeadsetType::None;
	bool useSlamTracking = false;
	uint32_t resolutionX = 0;
	uint32_t resolutionY = 0;
	float cantingAngle = 0;
	float ipd = 0;
	// if the headset is directly connected instead of using the PVR API
	bool directConnected = false;
	bool hasEyeTracking = false;
	std::string headsetName;
};

class PimaxCommon {
public:
#ifdef PVR_EXISTS
	PimaxCommon();
	virtual ~PimaxCommon();
#else
	PimaxCommon() = default;
	virtual ~PimaxCommon() = default;
#endif
	static PimaxInfo GetInfo();
	static bool TryDirectConnection();
	static bool IsPimaxLighthouseDevice(std::string_view model, std::string_view manufacturer);
	static bool IsLighthouseHeadsetConnected();
	static bool IsSlamHeadsetConnected();
#ifdef PVR_EXISTS
	static pvrSessionHandle GetPvrSession(bool forceTryConnect = false);
	static double GetPvrTime();
#endif
	static Config::BaseHeadsetConfig& PatchConfig(Config::BaseHeadsetConfig& config);
	static Config::PimaxHeadsetConfig& GetHeadsetConfig();
	static Config::PimaxHeadsetConfig& GetHeadsetConfigOld();
	static Config::PimaxHeadsetConfig& GetHeadsetConfigDefault();
	static std::shared_mutex pvrLock;
protected:
#ifdef PVR_EXISTS
	pvrHmdInfo GetHmdInfo() const { return hmdInfo; };
	bool HasEyeTracking() const;

	bool CheckPvrDeviceLost();

	void StartEyeTracking();
	void StopEyeTracking();

	void SetVisibilityMeshes();

	void PollMagicAttach();
	void SetSceneApplicationProcess(uint32_t pid);
#endif

	void GetHmdButtonsState(bool& systemButton, bool& doubleTap);

private:
	virtual void RunPvrBackground() {}
#ifdef PVR_EXISTS
	void EyeTrackingThread();

	pvrHmdInfo hmdInfo = {};
	std::thread eyeTrackingThread;
	std::atomic<bool> eyeTrackingRunning;
	uint32_t lastSceneApplicationPid = 0;
	bool isLibMagicEnabled = false;

	friend void PvrThread();
#endif
};

#ifdef PVR_EXISTS
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
#endif
