#include "PimaxCommon.h"

#include <PVR_Math.h>

#include "../Driver/DriverLog.h"
#include "EyeTrackingOutput.h"

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
		s_info.connected = false;
		if (pvr_createSession(s_pvr, &s_pvrSession) != pvr_success) {
			return false;
		}
	}

	if (s_pvrSession && !s_info.connected) {
		pvrHmdStatus hmdStatus = {};
		pvr_getHmdStatus(s_pvrSession, &hmdStatus);
		if (!(hmdStatus.ServiceReady && hmdStatus.HmdPresent && !hmdStatus.ShouldQuit)) {
			return false;
		}

		pvrHmdInfo info = {};
		pvr_getHmdInfo(s_pvrSession, &info);
		const std::string_view productName = info.ProductName;
		pvrDisplayInfo displayInfo = {};
		pvr_getEyeDisplayInfo(s_pvrSession, pvrEye_Left, &displayInfo);
		switch (info.ProductId) { // ProductId comes from the USB device
		case 0x0044: // Dream Air
			s_info.headsetType = Config::HeadsetType::DreamAir;
			break;
		case 0x0042: // Dream Air SE
			s_info.headsetType = Config::HeadsetType::DreamAirSE;
			break;
		case 0x0040: // Crystal Super (50PPD, 57PPD, Ultrawide, MicroOLED share this ProductId)
			// TODO: Find a better way to differenciate these.
			if (displayInfo.width == 7104) {
				s_info.headsetType = Config::HeadsetType::CrystalSuperMicroOLED;
			}
			else {
				s_info.headsetType = Config::HeadsetType::CrystalSuper50PPD;
			}
			break;
		case 0x0018: // Crystal Light
			s_info.headsetType = Config::HeadsetType::CrystalLight;
			break;
		case 0x0012: // Crystal OG
			s_info.headsetType = Config::HeadsetType::CrystalOG;
			break;
		case 0x0101: // Pimax 5K and 8K series (share the same ProductId)
			if (productName.find("5K") != std::string::npos) {
				s_info.headsetType = Config::HeadsetType::Pimax5KPlus;
			}
			else if (productName.find("Artisan") != std::string::npos) {
				s_info.headsetType = Config::HeadsetType::PimaxArtisan;
			}
			else {
				s_info.headsetType = Config::HeadsetType::Pimax8KX;
			}
			break;
		default:
			break;
		}
		if(info.ProductId){
			DriverLog("Detected PVR headset '%s' (%04x)%s", info.ProductName, info.ProductId, s_info.headsetType == Config::HeadsetType::None ? " - not supported" : "");
		}

		// This is what the OpenPort toggle sets in Pimax EVO.
		const bool isOpenPortEnabled = pvr_getIntConfig(s_pvrSession, "no_render", 0);

		// Filter by: 1) is OpenPort enabled, 2) do we support the attached headset?
		s_info.connected = isOpenPortEnabled && s_info.headsetType != Config::HeadsetType::None;
		if (s_info.connected) {
			pvrHmdTrackingStyle trackingStyle = pvrHmdTrackingStyle_Unknown;
			trackingStyle = (pvrHmdTrackingStyle)pvr_getTrackedDeviceIntProperty(
				s_pvrSession,
				pvrTrackedDevice_HMD,
				pvrTrackedDeviceProp_Prop_HmdTrackingStyle_Int,
				pvrHmdTrackingStyle_Unknown);
			s_info.useSlamTracking = trackingStyle == pvrHmdTrackingStyle_InsideOutCameras;

			DriverLog("Detected headset '%s' (%04x) with %s tracking", info.ProductName, info.ProductId, s_info.useSlamTracking ? "SLAM" : "Lighthouse");
			DriverLog("Panel Resolution: %ux%u (Orientation: %u deg)", displayInfo.width, displayInfo.height, displayInfo.eye_rotate * 90);
			s_info.resolutionX = displayInfo.width / 2;
			s_info.resolutionY = displayInfo.height;
			pvrEyeRenderInfo eyeInfo[pvrEye_Count] = {};
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Left, &eyeInfo[pvrEye_Left]);
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Right, &eyeInfo[pvrEye_Right]);
			const auto cantingAngle = PVR::Quatf{ eyeInfo[pvrEye_Left].HmdToEyePose.Orientation }.Angle(eyeInfo[pvrEye_Right].HmdToEyePose.Orientation) /
				2.f;
			DriverLog("Canting Angle: %.2f deg", cantingAngle * 180 / 3.1415926f);
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

Config::BaseHeadsetConfig& PimaxCommon::PatchConfig(Config::BaseHeadsetConfig& config) {
	if (config.resolutionX == 0 || config.resolutionY == 0) {
		config.resolutionX = GetInfo().resolutionX;
		config.resolutionY = GetInfo().resolutionY;
	}
	return config;
}

bool PimaxCommon::CheckDeviceLost() {
	pvrHmdStatus hmdStatus = {};
	pvr_getHmdStatus(GetPvrSession(), &hmdStatus);
	const bool deviceReady = hmdStatus.ServiceReady && hmdStatus.HmdPresent && !hmdStatus.ShouldQuit;
	if (s_info.connected && !deviceReady) {
		DriverLog("Detected loss of connection to the headset");
		StopEyeTracking();
		pvr_destroySession(GetPvrSession());
		s_info.connected = false;
		s_pvrSession = {};
	}
	return !deviceReady;
}

void PimaxCommon::StartEyeTracking() {
	if (s_pvrSession && !eyeTrackingRunning.exchange(true)) {
		DriverLog("Starting eye tracking thread");
		eyeTrackingThread = std::thread(&PimaxCommon::EyeTrackingThread, this);
	}
}

void PimaxCommon::StopEyeTracking() {
	if (eyeTrackingRunning.exchange(false) && eyeTrackingThread.joinable()) {
		DriverLog("Stopping eye tracking thread");
		eyeTrackingThread.join();
		eyeTrackingThread = {};
	}
}

void PimaxCommon::EyeTrackingThread() {
	const HANDLE timer = CreateWaitableTimer(nullptr, false, nullptr);
	const LARGE_INTEGER noDelay = {};
	const LONG periodMs = 1000 / 120; // Approx 120Hz
	SetWaitableTimer(timer, &noDelay, periodMs, nullptr, nullptr, true);

	while (true) {
		WaitForSingleObject(timer, 100);
		if (!eyeTrackingRunning) {
			break;
		}

		pvrEyeTrackingInfo eyeTrackingInfo = {};
		pvr_getEyeTrackingInfo(GetPvrSession(), GetPvrTime(), &eyeTrackingInfo);
		if (eyeTrackingInfo.TimeInSeconds) {
			// Convert PVR eye tracking data to angles for EyeTrackingOutput.
			// PVR provides gaze as tangent values, convert to angles in radians.
			const float leftAngleX = atanf(eyeTrackingInfo.GazeTan[pvrEye_Left].x);
			const float leftAngleY = atanf(eyeTrackingInfo.GazeTan[pvrEye_Left].y);
			const float rightAngleX = atanf(eyeTrackingInfo.GazeTan[pvrEye_Right].x);
			const float rightAngleY = atanf(eyeTrackingInfo.GazeTan[pvrEye_Right].y);
			eyeTrackingOutput.SetEyeTrackingData(leftAngleX, leftAngleY, rightAngleX, rightAngleY);
		}
	}

	CloseHandle(timer);

	DriverLog("Eye tracking thread stopped");
}
