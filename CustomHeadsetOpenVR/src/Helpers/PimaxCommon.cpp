#include "PimaxCommon.h"

#include <PVR_Math.h>

#include "../Driver/DriverLog.h"
#include "EyeTrackingOutput.h"

// For "hmd_buttons" config. Not in PVR SDK.
enum class HmdButton : int {
	Button_Power = 0x0001,
	Button_VolumeUp = 0x0002,
	Button_VolumeDown = 0x0004,
	Button_DoubleTap = 0x0008,
};
DEFINE_ENUM_FLAG_OPERATORS(HmdButton);

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
			else if (displayInfo.edid_vid == 21594){
				// TODO: remove this when a better detection method is found
				s_info.headsetType = Config::HeadsetType::CrystalSuperUltrawide;
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
		if (info.ProductId) {
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
				
			// Always query without parallel projection enabled.
			// The underlying driver will then (re)apply parallel projection if needed during Activate() and/or RunFrame().
			pvr_setIntConfig(s_pvrSession, "view_rotation_fix", 0);
			pvrEyeRenderInfo eyeInfo[pvrEye_Count] = {};
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Left, &eyeInfo[pvrEye_Left]);
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Right, &eyeInfo[pvrEye_Right]);
			s_info.ipd = PVR::Vector3f(eyeInfo[pvrEye_Left].HmdToEyePose.Position).Distance(eyeInfo[pvrEye_Right].HmdToEyePose.Position) * 1000;
			DriverLog("IPD: %.1f mm", s_info.ipd);
			s_info.cantingAngle = PVR::Quatf{ eyeInfo[pvrEye_Left].HmdToEyePose.Orientation }.Angle(eyeInfo[pvrEye_Right].HmdToEyePose.Orientation) / 2.f;
			s_info.cantingAngle *= 180 / 3.1415926f;
			DriverLog("Canting Angle: %.2f deg", s_info.cantingAngle);
		}
	}
	return true;
}

PimaxInfo PimaxCommon::GetInfo() {
	EnsurePvrSession();
	return s_info;
}

bool PimaxCommon::IsLighthouseHeadsetConnected(){
	return PimaxCommon::GetInfo().headsetType && driverConfig.ConfigFromHeadsetType(PimaxCommon::GetInfo().headsetType)->enable && PimaxCommon::GetInfo().connected && !PimaxCommon::GetInfo().useSlamTracking;
}

bool PimaxCommon::IsSlamHeadsetConnected(){
	return PimaxCommon::GetInfo().headsetType && driverConfig.ConfigFromHeadsetType(PimaxCommon::GetInfo().headsetType)->enable && PimaxCommon::GetInfo().connected && PimaxCommon::GetInfo().useSlamTracking;
}

pvrSessionHandle PimaxCommon::GetPvrSession() {
	EnsurePvrSession();
	return s_pvrSession;
}

double PimaxCommon::GetPvrTime() {
	return pvr_getTimeSeconds(s_pvr);
}

Config::BaseHeadsetConfig& PimaxCommon::PatchConfig(Config::BaseHeadsetConfig& config) {
	if (config.resolutionX == 0 || config.resolutionY == 0) {
		config.resolutionX = GetInfo().resolutionX;
		config.resolutionY = GetInfo().resolutionY;
	}
	if (config.hardwareIpd) {
		config.ipd = GetInfo().ipd;
	}
	if (config.autoEyeRotation) {
		if (config.distortionProfile == "Pimax Builtin" && config.parallelProjection) {
			config.eyeRotation = 0;
		}
		else {
			config.eyeRotation = GetInfo().cantingAngle;
		}
	}
	return config;
}

Config::BaseHeadsetConfig& PimaxCommon::GetHeadsetConfig(){
	Config::BaseHeadsetConfig* config = driverConfig.ConfigFromHeadsetType(GetInfo().headsetType);
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &driverConfig.dreamAir;
	}
	return PatchConfig(*config);
}

Config::BaseHeadsetConfig& PimaxCommon::GetHeadsetConfigOld(){
	Config::BaseHeadsetConfig* config = driverConfigOld.ConfigFromHeadsetType(GetInfo().headsetType);
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &driverConfigOld.dreamAir;
	}
	return PatchConfig(*config);
}

PimaxCommon::PimaxCommon() {
	// Cache useful immutable state.
	pvr_getHmdInfo(GetPvrSession(), &hmdInfo);
	hasEyeTracking = // Crystal OG, Crystal Super, Dream Air SE, Dream Air.
		GetHmdInfo().ProductId == 0x0012 || GetHmdInfo().ProductId == 0x0040 ||
		GetHmdInfo().ProductId == 0x0042 || GetHmdInfo().ProductId == 0x0044;
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

void PimaxCommon::SetVisibilityMeshes() {
	vr::CVRHiddenAreaHelpers helpers = { vr::VRPropertiesRaw() };
	for (int eye = 0; eye < pvrEye_Count; eye++) {
		std::vector<vr::HmdVector2_t> vertices;
		size_t count;

		count = pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_HiddenArea, nullptr, 0);
		vertices.resize(count);
		pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_HiddenArea,
			(pvrVector2f*)vertices.data(), (unsigned int)vertices.size());
		helpers.SetHiddenArea((vr::EVREye)eye, vr::k_eHiddenAreaMesh_Standard, vertices.data(), (uint32_t)vertices.size());

		count = pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_VisibleArea, nullptr, 0);
		vertices.resize(count);
		pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_VisibleArea,
			(pvrVector2f*)vertices.data(), (unsigned int)vertices.size());
		helpers.SetHiddenArea((vr::EVREye)eye, vr::k_eHiddenAreaMesh_Inverse, vertices.data(), (uint32_t)vertices.size());
	}
}

void PimaxCommon::PollMagicAttach() {
	// Detect if Pimax LibMagic (DFR injector) was enabled after a scene application started and re-assert the
	// PID of the current scene application.
	const bool wasLibMagicEnabled = isLibMagicEnabled;
	isLibMagicEnabled = pvr_getIntConfig(GetPvrSession(), "enable_foveated_rendering", 0);
	if (isLibMagicEnabled != wasLibMagicEnabled) {
		SetSceneApplicationProcess(lastSceneApplicationPid);
	}
}

void PimaxCommon::SetSceneApplicationProcess(uint32_t pid) {
	if (pid) {
		// Signal Pimax Play to perform a MagicAttach (DFR injector) when a new scene app started.
		pvr_setIntConfig(GetPvrSession(), "openvr_client_changed", pid);
	}
	lastSceneApplicationPid = pid;
}

void PimaxCommon::GetHmdButtonsState(bool& systemButton, bool& doubleTap) {
	const HmdButton hmdButtonsState = (HmdButton)pvr_getIntConfig(GetPvrSession(), "hmd_buttons", 0);
	const auto isButtonPressed = [&hmdButtonsState](const HmdButton button) {
		return (hmdButtonsState & button) == button;
		};
	systemButton = isButtonPressed(HmdButton::Button_VolumeUp) && isButtonPressed(HmdButton::Button_VolumeDown);
	doubleTap = isButtonPressed(HmdButton::Button_DoubleTap);
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
