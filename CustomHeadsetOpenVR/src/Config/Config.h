#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <tuple>

struct HiddenAreaMeshConfig {
	bool enable = false;
	bool testMode = false;
	int detailLevel = 8;
	double radiusTopOuter = 0.25;
	double radiusTopInner = 0.25;
	double radiusBottomInner = 0.25;
	double radiusBottomOuter = 0.25;

	constexpr bool operator==(const HiddenAreaMeshConfig& other) const {
		return std::tie(this->enable, this->testMode, this->detailLevel, this->radiusTopOuter, this->radiusTopInner, this->radiusBottomInner, this->radiusBottomOuter) ==
		       std::tie(other.enable, other.testMode, other.detailLevel, other.radiusTopOuter, other.radiusTopInner, other.radiusBottomInner, other.radiusBottomOuter);
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
	double chroma = 50;
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
	// a 3x3 matrix to apply to the linear colors
	// if this is an array of 9 flat elements it will override the headset's default matrix
	std::vector<double> srgbColorCorrectionMatrix = {};
	
};


class Config{
public:
	class Color{
	public:
		double r = 1.0;
		double g = 1.0;
		double b = 1.0;
	};
	class MeganeX8KConfig{
	public:
		// if the MeganeX superlight 8K should be shimmed byt this driver
		bool enable = true;
		// ipd in mm
		double ipd = 63.0;
		// ipd offset from the ipd value in mm
		double ipdOffset = 0.0;
		// minimum black levels from 0 to 1
		double blackLevel = 0;
		// tint the display this color
		Color colorMultiplier = {};
		// distortion profile to use
		std::string distortionProfile = "MeganeX8K Default";
		// amount to zoom in the distortion profile
		double distortionZoom = 1.0;
		// amount to shift the subpixels to account for their diffent rows
		double subpixelShift = 0.33;
		// width of one eye in pixels
		int resolutionX = 3840;
		// height of one eye in pixels
		int resolutionY = 3552;
		// max horizontal fov
		double maxFovX = 100.0;
		// max vertical fov
		double maxFovY = 96.0;
		// distortion mesh resolution
		int distortionMeshResolution = 127;
		// if the fov should be slightly adjusted each session to prevent sharp burn in along the edges
		bool fovBurnInPrevention = true;
		// multiply 100% render resolution width
		double renderResolutionMultiplierX = 1.0;
		// multiply 100% render resolution height
		double renderResolutionMultiplierY = 1.0;
		// seconds of latency to the display
		double secondsFromVsyncToPhotons = 0.011;
		// Config struct for the hidden area mesh
		HiddenAreaMeshConfig hiddenArea;
		// config for dimming the display when stationary
		StationaryDimmingConfig stationaryDimming = {};
	};
	// config for the MeganeX superlight 8K
	MeganeX8KConfig meganeX8K;
	
	CustomShaderConfig customShader = {};
	
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
	// description to display
	std::string description = "";
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
	// amount to smooth the curve from 0 to 1
	double smoothAmount = 0.66;
};

// global config object
extern Config driverConfig;

// config from before the last reload
extern Config driverConfigOld;

// lock for the config to prevent updates while reading
extern std::mutex driverConfigLock;

// version of the application
extern std::string driverVersion;