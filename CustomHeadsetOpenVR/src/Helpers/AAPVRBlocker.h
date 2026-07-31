// This file is for temporary testing only and will eventually be removed once the AAPVR driver is properly updated.

// Although partially blocking the AAPVR driver so that only slam tracked controllers are allowed through sort of works. It still has problems that it the driver does not realize that devices have been blocked and continues to try and send updates about them.
// When defined, this will simply block the driver from loading, similarly to how it blocks the lighthouse driver.
#define FULLY_BLOCK_AAPVR

#pragma once
#include "../Driver/DeviceShim.h"

// Unblock the lighthouse driver from AAPVR's LoadLibrary hook by pre-loading
// the DLL and intercepting subsequent LoadLibrary calls to return our handle.
// Due to unsolvable problems, instead it just completely blocks the aapvr driver from loading.
// These are Windows-only features.
#ifdef _WIN32
void AAPVRLighthouseUnblockerLoadDriver();
void AAPVRLighthouseUnblockerInjectHooks();
void AAPVRLighthouseUnblockerRemoveHooks();
bool AAPVRShouldBlock();
#else
inline void AAPVRLighthouseUnblockerLoadDriver() {}
inline void AAPVRLighthouseUnblockerInjectHooks() {}
inline void AAPVRLighthouseUnblockerRemoveHooks() {}
inline bool AAPVRShouldBlock() { return false; }
#endif


class APPVRDeviceBlocker : public ShimDefinition{
public:
	// override the activation function
	virtual void PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue) override;
};