#include "PimaxCommon.h"

#include <PVR_Math.h>

#include "../Driver/DriverLog.h"

static pvrEnvHandle s_pvr = {};
static pvrSessionHandle s_pvrSession = {};
static PimaxInfo s_info = {};

static bool EnsurePvrSession() {
	if (!s_pvr) {
		if (pvr_initialise(&s_pvr) != pvr_success) {
			return false;
		}
	}
	if (!s_pvrSession) {
		if (pvr_createSession(s_pvr, &s_pvrSession) != pvr_success) {
			return false;
		}

		pvrHmdInfo info = {};
		pvr_getHmdInfo(s_pvrSession, &info);
		// TODO: Add other Pimax headsets here.
		switch (info.ProductId) {
		case 0x0044:
			s_info.headsetType = DreamAir;
			break;
		}

		// This is what the OpenPort toggle sets in Pimax EVO.
		const bool isOpenPortEnabled = pvr_getIntConfig(s_pvrSession, "no_render", 0);

		// Filter by: 1) is OpenPort enabled, 2) do we support the attached headset?
		s_info.connected = isOpenPortEnabled && s_info.headsetType != Invalid;
		if (s_info.connected) {
			pvrHmdTrackingStyle trackingStyle = pvrHmdTrackingStyle_Unknown;
			trackingStyle = (pvrHmdTrackingStyle)pvr_getTrackedDeviceIntProperty(
				s_pvrSession,
				pvrTrackedDevice_HMD,
				pvrTrackedDeviceProp_Prop_HmdTrackingStyle_Int,
				pvrHmdTrackingStyle_Unknown);
			s_info.useSlamTracking = trackingStyle == pvrHmdTrackingStyle_InsideOutCameras;

			DriverLog("Detected headset '%s' (%04x) with %s tracking", info.ProductName, info.ProductId, s_info.useSlamTracking ? "SLAM" : "Lighthouse");
		}
	}
	return true;
}

PimaxInfo PimaxCommon::GetInfo() {
	EnsurePvrSession();
	return s_info;
}

pvrSessionHandle PimaxCommon::GetPvrSession() {
	EnsurePvrSession();
	return s_pvrSession;
}

double PimaxCommon::GetPvrTime() {
	return pvr_getTimeSeconds(s_pvr);
}

PimaxCommon::PimaxCommon() {
	// Cache useful immutable state.
	pvr_getHmdInfo(GetPvrSession(), &hmdInfo);
	hasEyeTracking = // Crystal OG, Crystal Super, Dream Air SE, Dream Air.
		GetHmdInfo().ProductId == 0x0012 || GetHmdInfo().ProductId == 0x0040 ||
		GetHmdInfo().ProductId == 0x0042 || GetHmdInfo().ProductId == 0x0044;
}
