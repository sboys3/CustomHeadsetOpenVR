#pragma once
#include <PVR.h>
#include <PVR_API.h>
#include <string>

enum PimaxHeadsetType {
	DreamAir = 0,

	Invalid
};

struct PimaxInfo {
	bool connected = false;
	PimaxHeadsetType headsetType = Invalid;
	bool useSlamTracking = false;
};

class PimaxCommon {
public:
	PimaxCommon();
	virtual ~PimaxCommon() = default;
	static PimaxInfo GetInfo();

protected:
	static pvrSessionHandle GetPvrSession();
	static double GetPvrTime();
	pvrHmdInfo GetHmdInfo() const { return hmdInfo; };
	bool HasEyeTracking() const { return hasEyeTracking; }

private:
	pvrHmdInfo hmdInfo = {};
	bool hasEyeTracking = false;
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
