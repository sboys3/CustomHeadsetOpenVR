#pragma once
#include <PVR.h>
#include <PVR_API.h>

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
