#pragma once
#include <PVR.h>
#include <PVR_API.h>

class PimaxCommon {
public:
	PimaxCommon();
	virtual ~PimaxCommon() = default;

protected:
	pvrSessionHandle GetPvrSession() const { return session; };
	bool IsOpenPortEnabled() const { return isOpenPortEnabled; }
	pvrHmdInfo GetHmdInfo() const { return hmdInfo; };

private:
	pvrEnvHandle pvr = {};
	pvrSessionHandle session = {};
	bool isOpenPortEnabled = false;
	pvrHmdInfo hmdInfo = {};
};
