#pragma once
#include <string>
#include <vector>
#include <mutex>


class Config{
public:
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
	};
	// config for the MeganeX superlight 8K
	MeganeX8KConfig meganeX8K;
	
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