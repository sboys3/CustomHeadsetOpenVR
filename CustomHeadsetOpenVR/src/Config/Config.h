#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <tuple>

#if !defined(VENDOR_PIMAX) && !defined(VENDOR_SHIFTALL)
#define VENDOR_NEUTRAL
// #error "this should not be neutral"
#endif

struct ConfigColor{
	double r = 1.0;
	double g = 1.0;
	double b = 1.0;
};

struct HiddenAreaMeshConfig {
	bool enable = false;
	bool testMode = false;
	int detailLevel = 8;
	double radiusTopOuter = 0.25;
	double radiusTopInner = 0.25;
	double radiusBottomInner = 0.25;
	double radiusBottomOuter = 0.25;
	bool autoHiddenArea = false;

	constexpr bool operator==(const HiddenAreaMeshConfig& other) const {
		return std::tie(this->enable, this->testMode, this->detailLevel, this->radiusTopOuter, this->radiusTopInner, this->radiusBottomInner, this->radiusBottomOuter, this->autoHiddenArea) ==
		       std::tie(other.enable, other.testMode, other.detailLevel, other.radiusTopOuter, other.radiusTopInner, other.radiusBottomInner, other.radiusBottomOuter, other.autoHiddenArea);
	}
	constexpr bool operator!=(const HiddenAreaMeshConfig& other) const {
		return !(this->operator==(other));
	}
};

struct StationaryDimmingConfig{
	// if the display should be dimmed when the headset is stationary
	bool enable = true;
	// the angle that the headset has to rotate for it to be considered as moved
	double movementThreshold = 0.4;
	// the time in seconds that the headset has to be stationary for it to be dimmed
	double movementTime = 15.0;
	// the amount to dim the display to when stationary
	double dimBrightnessPercent = 2;
	// the amount per second to dim the display when stationary
	double dimSeconds = 10;
	// the amount per second to brighten the display when moving
	double brightenSeconds = 5;
};


struct CustomShaderConfig{
	// if shaders should be replaced in the compositor
	bool enable = false;
	bool enableForMeganeX8K = true;
	bool enableForDreamAir = true;
	bool enableForOther = false;
	// contrast with 50 being normal
	double contrast = 50;
	// the point from 0-100% of white that the contrast is centered around
	double contrastMidpoint = 50;
	// if the contrast should be done in linear space instead of gamma
	bool contrastLinear = false;
	// if per eye contrast should be applied
	bool contrastPerEye = false;
	bool contrastPerEyeLinear = false;
	double contrastLeft = 50;
	double contrastMidpointLeft = 50;
	double contrastRight = 50;
	double contrastMidpointRight = 50;
	// increase or decrease the variation of the colors
	double saturation = 50;
	// gamma of the output
	double gamma = 2.2;
	// if the subpixels should be offset
	bool subpixelShift = true;
	// if the mura correction should be skipped
	bool disableMuraCorrection = false;
	// if the black levels should be skipped
	bool disableBlackLevels = false;
	// if the colors should be corrected to display the srgb input as srgb on the display
	bool srgbColorCorrection = false;
	// if the white point correction should be applied to the srgb color correction
	bool srgbWhitePointCorrection = false;
	// a 3x3 matrix to apply to the linear colors
	// if this is an array of 9 flat elements it will override the headset's default matrix
	std::vector<double> srgbColorCorrectionMatrix = {};
	// correct color uniformity issues of the lenses on the MeganeX
	bool lensColorCorrection = true;
	// if a 10 bit input will be dithered down to 8 bit
	bool dither10Bit = false;
	// if the filter should be enabled for overlays (defaults false to avoid performance hit when no overlay is shown)
	bool enableFilterForOverlay = false;
	// if the filter should be enabled when the SteamVR dashboard is open
	bool enableFilterForDashboard = true;
	// filters on the sampling of the texture,  "None", "NearestNeighbor", "FXAA2", "FXAA2CAS", "LumaSharpen", and "CAS"
	std::string samplingFilter = "None";
	// FXAA2 filter parameters
	double samplingFilterFXAA2SharpenStrength = 1.0;
	double samplingFilterFXAA2SharpenClamp = 0.05;
	// FXAA2CAS filter parameters
	double samplingFilterFXAA2CASStrength = 1.0;
	double samplingFilterFXAA2CASContrast = 1.0;
	// luma sharpen filter parameters
	double samplingFilterLumaSharpenStrength = 2.0;
	double samplingFilterLumaSharpenClamp = 0.1;
	int samplingFilterLumaSharpenPattern = 1;
	double samplingFilterLumaSharpenRadius = 1.0;
 	// CAS filter parameters
 	double samplingFilterCASStrength = 1.0;
 	double samplingFilterCASContrast = 1.0;
	// color multiplier for tint adjustments
	ConfigColor colorMultiplier = {1.0, 1.0, 1.0};
};


class Config{
public:
	enum HeadsetType{
		None = 0,
		Other = 1,
		MeganeX8K = 2,
		Vive = 3,
		DreamAir = 4,
		DreamAirSE = 5,
		CrystalSuper50PPD = 6,
		CrystalSuper57PPD = 7,
		CrystalSuperUltrawide = 8,
		CrystalSuperMicroOLED = 9,
		CrystalLight = 14,
		CrystalOG = 15,
		Pimax5KSuper = 16,
		Pimax5KPlus = 17,
		Pimax8KX = 18,
		Pimax8KPlus = 19,
		PimaxArtisan = 20,
	};
	static inline bool IsPimaxHeadset(HeadsetType type){
		return type == HeadsetType::DreamAir ||
		       type == HeadsetType::DreamAirSE ||
		       type == HeadsetType::CrystalSuper50PPD ||
		       type == HeadsetType::CrystalSuper57PPD ||
		       type == HeadsetType::CrystalSuperUltrawide ||
		       type == HeadsetType::CrystalSuperMicroOLED ||
		       type == HeadsetType::CrystalLight ||
		       type == HeadsetType::CrystalOG ||
		       type == HeadsetType::Pimax5KSuper ||
		       type == HeadsetType::Pimax5KPlus ||
		       type == HeadsetType::Pimax8KX ||
		       type == HeadsetType::Pimax8KPlus ||
		       type == HeadsetType::PimaxArtisan;
	}
	static inline bool IsShiftallHeadset(HeadsetType type){
		return type == HeadsetType::MeganeX8K;
	}
	
	class BaseHeadsetConfig{
	public:
		// if the headset should be shimmed by this driver
		bool enable = true;
		// the type of headset this is
		HeadsetType headsetType = HeadsetType::None;
		// ipd in mm
		double ipd = 63.0;
		// ipd offset from the ipd value in mm
		double ipdOffset = 0.0;
		// whether to use ipd data from the platform (eg: PVR)
		bool autoIpd = false;
		// horizontal offset in mm to shift both eyes to the right
		double horizontalIPDOffset = 0.0;
		// minimum black levels from 0 to 1
		double blackLevel = 0;
		// tint the display this color
		ConfigColor colorMultiplier = {};
		// distortion profile to use
		std::string distortionProfile = "None";
		// amount to zoom in the distortion profile
		double distortionZoom = 1.0;
		// amount to zoom in the FOV, the fov is divided by this value
		double fovZoom = 1.0;
		// amount to zoom in the FOV using tangent-based scaling for flatter perception
		double flatFovZoom = 1.0;
		// multiplier for the subpixel offsets
		double subpixelShift = 1.0;
		// subpixel offsets in pixel units for each color channel [offsetXRed, offsetYRed, offsetXGreen, offsetYGreen, offsetXBlue, offsetYBlue]
		std::vector<double> subpixelOffsets = {0, 0, 0, 0, 0, 0};
		// width of one eye in pixels. 0: get data from the platform (eg: PVR)
		int resolutionX = 3840;
		// height of one eye in pixels 0: get data from the platform (eg: PVR)
		int resolutionY = 3552;
		// clockwise rotation of the image on the right display, 0:0, 1:90, 2:180, 3:270
		int displayRotation = 0;
		// max horizontal fov
		double maxFovX = 100.0;
		// max vertical fov
		double maxFovY = 96.0;
		// distortion mesh resolution
		int distortionMeshResolution = 127;
		// if the fov should be slightly adjusted each session to prevent sharp burn in along the edges
		bool fovBurnInPrevention = true;
		// if the distortion profile should clamp the image to the bounds of the display or if it will instead render an image at whatever FOV is set
		bool fovClamping = true;
		// device type used to filter distortion profiles in the GUI
		std::string distortionProfileDeviceType = "";
		// multiply 100% render resolution width
		double renderResolutionMultiplierX = 1.0;
		// multiply 100% render resolution height
		double renderResolutionMultiplierY = 1.0;
		// percent of 1:1 resolution to apply the super sampling downscale filter at, this is really high to allow for subpixel sampling
		double superSamplingFilterPercent = 500;
		// seconds of latency to the display
		double secondsFromVsyncToPhotons = 0.007;
		// seconds from the the first to last line of the display
		double secondsFromPhotonsToVblank = 0.0025;
		// angle in degrees for each eye to be rotated outwards
		double eyeRotation = 0.0;
		// whether to use eye rotation from the platform (eg: PVR)
		bool autoEyeRotation = false;
		// disable eyes as much as possible. 0:both enabled 1:left disabled 2:right disabled 3:both disabled
		int disableEye = 0;
		// if the fov should be decreased for the disabled eye, this causes problems in some apps
		bool disableEyeDecreaseFov = false;
		// if a vive link box should be used for bluetooth
		bool useViveBluetooth = false;
		// if the display is in direct mode or false if it is on the desktop
		bool directMode = true;
		// if the icons in the SteamVR status window should be modified
		bool replaceIcons = true;
		// the edid for the headset
		int edidVendorId = 0;
		// the edid for the headset
		int edidProductId = 0;
		// if non zero, override the edid vendor id
		int edidVendorIdOverride = 0;
		// if non zero, override the edid product id
		int edidProductIdOverride = 0;
		// DSC Version
		int dscVersion = -1;
		// DSC Slice count
		int dscSliceCount = -1;
		// DSC bits per pixel
		int dscBPPx16 = -1;
		// if the driver should be enabled for every hmd
		bool forceEnable = false;
		// if parallel projection should be used for rendering
		bool parallelProjection = true;
		// if eye tracking should be enabled
		bool enableEyeTracking = false;
		// Config struct for the hidden area mesh
		HiddenAreaMeshConfig hiddenArea;
		// config for dimming the display when stationary
		StationaryDimmingConfig stationaryDimming = {};
		// define virtual destructor to allow virtual function classes
		virtual ~BaseHeadsetConfig() = default;
	};
	class PimaxHeadsetConfig : public BaseHeadsetConfig{
		
	};
	
	class MeganeX8KConfig : public BaseHeadsetConfig{
	public:
		MeganeX8KConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_SHIFTALL)
			enable = false;
			#endif
			headsetType = HeadsetType::MeganeX8K;
			distortionProfile = "MeganeX8K Default";
			distortionProfileDeviceType = "MeganeX8K";
			edidVendorId = 0xcc4c; // SFL
			displayRotation = 1;
			subpixelOffsets = {-0.33 / 3552.0, 0, 0, 0, 0.33 / 3552.0, 0};
		}
	};
	// config for the MeganeX superlight 8K
	MeganeX8KConfig meganeX8K = {};
	
	class DreamAirConfig : public PimaxHeadsetConfig{
		public:
		DreamAirConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::DreamAir;
			distortionProfile = "Dream Air Default";
			distortionProfileDeviceType = "DreamAir";
			maxFovX = 96;
			maxFovY = 86;
			edidVendorId = 53826; // PVR
			displayRotation = 3;
			subpixelOffsets = {0.33 / 3552.0, 0, 0, 0, -0.33 / 3552.0, 0};
			eyeRotation = 2;
			enableEyeTracking = true;
		}
	};
	// config for the Dream Air
	DreamAirConfig dreamAir = {};
	
	// config for the Dream Air SE
	class DreamAirSEConfig : public PimaxHeadsetConfig{
		public:
		DreamAirSEConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::DreamAirSE;
			distortionProfile = "Dream Air Default";
			distortionProfileDeviceType = "DreamAir"; // this should have it's own distortion profiles as they are slightly different
			maxFovX = 86;
			maxFovY = 86;
			edidVendorId = 53826; // PVR
			displayRotation = 1;
			// WTF did they do with my 16 pixels?
			resolutionX = 2544;
			resolutionY = 2544;
			eyeRotation = 2.5;
			enableEyeTracking = true;
		}
	};
	DreamAirSEConfig dreamAirSE = {};

	// config for the Crystal Super 50PPD
	class CrystalSuper50PPDConfig : public PimaxHeadsetConfig{
		public:
		CrystalSuper50PPDConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::CrystalSuper50PPD;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "CrystalSuper50PPD";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 0;
			resolutionX = 3840;
			resolutionY = 3744;
			eyeRotation = 5;
			enableEyeTracking = true;
		}
	};
	CrystalSuper50PPDConfig crystalSuper50PPD = {};

	// config for the Crystal Super 57PPD
	class CrystalSuper57PPDConfig : public PimaxHeadsetConfig{
		public:
		CrystalSuper57PPDConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::CrystalSuper57PPD;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "CrystalSuper57PPD";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 21594;
			displayRotation = 0;
			resolutionX = 3840;
			resolutionY = 3744;
			eyeRotation = 7;
			enableEyeTracking = true;
		}
	};
	CrystalSuper57PPDConfig crystalSuper57PPD = {};

	// config for the Crystal Super Ultrawide
	class CrystalSuperUltrawideConfig : public PimaxHeadsetConfig{
		public:
		CrystalSuperUltrawideConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::CrystalSuperUltrawide;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "CrystalSuperUltrawide";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 21594;
			displayRotation = 0;
			resolutionX = 3840;
			resolutionY = 3744;
			eyeRotation = 7;
			enableEyeTracking = true;
		}
	};
	CrystalSuperUltrawideConfig crystalSuperUltrawide = {};

	// config for the Crystal Super MicroOLED
	class CrystalSuperMicroOLEDConfig : public PimaxHeadsetConfig{
		public:
		CrystalSuperMicroOLEDConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::CrystalSuperMicroOLED;
			distortionProfile = "Dream Air Default";
			distortionProfileDeviceType = "DreamAir";
			maxFovX = 96;
			maxFovY = 86;
			edidVendorId = 53826; // PVR
			displayRotation = 3;
			subpixelOffsets = {0.33 / 3552.0, 0, 0, 0, -0.33 / 3552.0, 0};
			eyeRotation = 3;
			enableEyeTracking = true;
		}
	};
	CrystalSuperMicroOLEDConfig crystalSuperMicroOLED = {};

	// config for the Crystal Light
	class CrystalLightConfig : public PimaxHeadsetConfig{
		public:
		CrystalLightConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::CrystalLight;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "CrystalLight";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 1;
			resolutionX = 2880;
			resolutionY = 2880;
			eyeRotation = 4;
		}
	};
	CrystalLightConfig crystalLight = {};

	// config for the Crystal OG
	class CrystalOGConfig : public PimaxHeadsetConfig{
		public:
		CrystalOGConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::CrystalOG;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "CrystalOG";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 3;
			enableEyeTracking = true;
		}
	};
	CrystalOGConfig crystalOG = {};

	// config for the Pimax 5K Super
	class Pimax5KSuperConfig : public PimaxHeadsetConfig{
		public:
		Pimax5KSuperConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::Pimax5KSuper;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "Pimax5KSuper";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 0;
			resolutionX = 0;
			resolutionY = 0;
			autoEyeRotation = true;
		}
	};
	Pimax5KSuperConfig pimax5KSuper = {};

	// config for the Pimax 5K Plus
	class Pimax5KPlusConfig : public PimaxHeadsetConfig{
		public:
		Pimax5KPlusConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::Pimax5KPlus;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "Pimax5KPlus";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 0;
			resolutionX = 0;
			resolutionY = 0;
			autoEyeRotation = true;
		}
	};
	Pimax5KPlusConfig pimax5KPlus = {};

	// config for the Pimax 8KX
	class Pimax8KXConfig : public PimaxHeadsetConfig{
		public:
		Pimax8KXConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::Pimax8KX;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "Pimax8KX";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 0;
			resolutionX = 0;
			resolutionY = 0;
			autoEyeRotation = true;
		}
	};
	Pimax8KXConfig pimax8KX = {};

	// config for the Pimax 8K Plus
	class Pimax8KPlusConfig : public PimaxHeadsetConfig{
		public:
		Pimax8KPlusConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::Pimax8KPlus;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "Pimax8KPlus";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 0;
			resolutionX = 0;
			resolutionY = 0;
			autoEyeRotation = true;
		}
	};
	Pimax8KPlusConfig pimax8KPlus = {};

	// config for the Pimax Artisan
	class PimaxArtisanConfig : public PimaxHeadsetConfig{
		public:
		PimaxArtisanConfig(){
			#if !defined(VENDOR_NEUTRAL) && !defined(VENDOR_PIMAX)
			enable = false;
			#endif
			headsetType = HeadsetType::PimaxArtisan;
			distortionProfile = "Pimax Builtin";
			distortionProfileDeviceType = "PimaxArtisan";
			maxFovX = 100;
			maxFovY = 90;
			edidVendorId = 53826; // PVR
			displayRotation = 0;
			resolutionX = 0;
			resolutionY = 0;
			autoEyeRotation = true;
		}
	};
	PimaxArtisanConfig pimaxArtisan = {};

	class FakeHeadsetConfig : public BaseHeadsetConfig{
		public:
		FakeHeadsetConfig(){
			enable = false;
			headsetType = HeadsetType::Other;
			distortionProfile = "MeganeX8K Default";
			displayRotation = 0;
			// use a 1080p monitor
			directMode = false;
			resolutionX = 960;
			resolutionY = 1080;
		}
	};
	// config for the fake headset
	FakeHeadsetConfig fakeHeadset = {};
	
	
	inline BaseHeadsetConfig* ConfigFromHeadsetType(HeadsetType headSetType){
		switch(headSetType){
			case HeadsetType::MeganeX8K:
				return &meganeX8K;
			case HeadsetType::DreamAir:
				return &dreamAir;
			case HeadsetType::DreamAirSE:
				return &dreamAirSE;
			case HeadsetType::CrystalSuper50PPD:
				return &crystalSuper50PPD;
			case HeadsetType::CrystalSuper57PPD:
				return &crystalSuper57PPD;
			case HeadsetType::CrystalSuperUltrawide:
				return &crystalSuperUltrawide;
			case HeadsetType::CrystalSuperMicroOLED:
				return &crystalSuperMicroOLED;
			case HeadsetType::CrystalLight:
				return &crystalLight;
			case HeadsetType::CrystalOG:
				return &crystalOG;
			case HeadsetType::Pimax5KSuper:
				return &pimax5KSuper;
			case HeadsetType::Pimax5KPlus:
				return &pimax5KPlus;
			case HeadsetType::Pimax8KX:
				return &pimax8KX;
			case HeadsetType::Pimax8KPlus:
				return &pimax8KPlus;
			case HeadsetType::PimaxArtisan:
				return &pimaxArtisan;
		}
	}
	
	class GeneralHeadsetConfig{
	public:
		// if a vive link box should be used for bluetooth
		bool useViveBluetooth = false;
	};
	GeneralHeadsetConfig generalHeadset = {};
	
	CustomShaderConfig customShader = {};
	
	// if devices should always be reported as tracking
	bool forceTracking = false;
	
	// if the screenshot requests should cause full compositor debug screenshots to be taken
	bool takeCompositorScreenshots = false;
	
	// makes the diver only do things related to closed source functionality if it exits
	// this allows for a driver built from source to run along side the driver with proprietary code
	bool onlyHandlePrivateFunctionality = false;
	
	// reload the config every time a file is changed in the distortions directory
	// this is for manual json editing, utilities should touch the main settings file when done modifying distortions instead
	// this is now enabled by default
	// bool watchDistortionProfiles = false;
	
	// if the config has been changes and should be reloaded
	// this will be set the false at the end of RunFrame
	bool hasBeenUpdated = true;
	
};

// config for a single custom distortion profile
class DistortionProfileConfig{
public:
	// name of distortion profile, this will be it's filename
	std::string name = "None";
	// the headset device this profile is for, empty for all devices, or "MeganeX8K" for the MeganeX superlight 8K
	std::string device = "";
	// description to display
	std::string description = "";
	// author of the distortion profile
	std::string author = "";
	// the date when it was created
	double creationDate = 0;
	// last time it was modified, used for reloading if changed
	double modifiedTime = 0;
	// type of distortion, None or RadialBezier
	std::string type = "None";
	// main distortion
	std::vector<double> distortions = {};
	// additional distortion to apply to the red channel
	std::vector<double> distortionsRed = {};
	// additional distortion to apply to the blue channel
	std::vector<double> distortionsBlue = {};
	// offset image outwards on the display using the same 0 to 100 scale 
	float offsetX = 0.0f;
	// offset image upwards on the display using the same 0 to 100 scale
	float offsetY = 0.0f;
	// if legacy smoothing should be used for bezier curves
	bool legacySmoothing = false;
	// amount to smooth the curve from 0 to 1 for legacy smoothing
	double smoothAmount = 0.66;
};

// global config object
extern Config driverConfig;

// config from before the last reload
extern Config driverConfigOld;

// config with default values
extern Config defaultDriverConfig;

// lock for the config to prevent updates while reading
extern std::mutex driverConfigLock;

// version of the application
extern std::string driverVersion;


#if __has_include("../Driver/HidModifierPrivate.cpp")
#define HAS_PRIVATE 1
#endif