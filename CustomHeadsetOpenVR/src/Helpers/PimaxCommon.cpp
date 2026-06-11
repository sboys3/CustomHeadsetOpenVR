#include "PimaxCommon.h"

#include "../Driver/DriverLog.h"

PimaxCommon::PimaxCommon() {
	pvr_initialise(&pvr);
	if (pvr_createSession(pvr, &session) == pvr_success) {
		// Cache useful immutable state.
		isOpenPortEnabled = pvr_getIntConfig(GetPvrSession(), "no_render", 0);
		pvr_getHmdInfo(GetPvrSession(), &hmdInfo);
		pvrHmdTrackingStyle trackingStyle = pvrHmdTrackingStyle_Unknown;
		trackingStyle = (pvrHmdTrackingStyle)pvr_getTrackedDeviceIntProperty(
			GetPvrSession(),
			pvrTrackedDevice_HMD,
			pvrTrackedDeviceProp_Prop_HmdTrackingStyle_Int,
			pvrHmdTrackingStyle_Unknown);
		DriverLog("Detected headset '%s' (%04x) with %s tracking", GetHmdInfo().ProductName, GetHmdInfo().ProductId, trackingStyle == pvrHmdTrackingStyle_InsideOutCameras ? "SLAM" : "Lighthouse");
	}
}
