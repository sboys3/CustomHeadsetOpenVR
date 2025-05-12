#include "DistortionProfileConstructor.h"
#include "RadialBezierDistortionProfile.h"

bool DistortionProfileConstructor::LoadDistortionProfile(std::string name){
	
	DistortionProfileConfig config = {};
	
	if(name == "MeganeX8K Default"){
		config.name = name;
		config.modifiedTime = 0;
		config.description = "MeganeX8K default distortion profile";
		config.type = "RadialBezier";
		config.distortions = {
			0.00000f, 0.0f,
			10.0000f, 24.7f,
			20.0000f, 48.0f,
			30.0000f, 69.6f,
			35.0000f, 79.9f,
			40.0000f, 89.06f,
			45.0000f, 96.30f,
			48.3073f, 100.0f
		};
	}
	
	if(name == "MeganeX8K Original"){
		config.name = name;
		config.modifiedTime = 0;
		config.description = "MeganeX8K original distortion profile which is the same as simplehmd.";
		config.type = "RadialBezier";
		config.distortions = {
			00.0000f, 0.0f,
			10.0000f, 24.77952472f,
			20.0000f, 48.32328161f,
			30.0000f, 69.9136628f,
			35.0000f, 79.99462488f,
			40.0000f, 89.06057112f,
			45.0000f, 96.29634484f,
			48.3073f, 100.0f,
		};
	}
	
	if(config.name == "None"){
		DistortionProfileConfig configFromDisk = driverConfigLoader.ParseDistortionConfig(name);
		if(configFromDisk.name != "None"){
			config = configFromDisk;
		}
	}
	
	
	// check if the profile has not changed to avoid recreating it
	if(profile != nullptr && config.name == profileName && config.modifiedTime == profileModifiedTime){
		return false;
	}
	
	DistortionProfile* newProfile = nullptr;
		
	// construct RadialBezierDistortionProfile object from config
	if(config.type == "RadialBezier"){
		RadialBezierDistortionProfile* radialBezierProfile = new RadialBezierDistortionProfile();
		if(config.distortions.size() >= 2){
			radialBezierProfile->distortions.clear();
			for(int i = 0; i < config.distortions.size() / 2; i++){
				radialBezierProfile->distortions.push_back({(float)config.distortions[i * 2], (float)config.distortions[i * 2 + 1]});
			}
		}
		if(config.distortionsRed.size() >= 2){
			radialBezierProfile->distortionsRed.clear();
			for(int i = 0; i < config.distortionsRed.size() / 2; i++){
				radialBezierProfile->distortionsRed.push_back({(float)config.distortionsRed[i * 2], (float)config.distortionsRed[i * 2 + 1]});
			}
		}
		if(config.distortionsBlue.size() >= 2){
			radialBezierProfile->distortionsBlue.clear();
			for(int i = 0; i < config.distortionsBlue.size() / 2; i++){
				radialBezierProfile->distortionsBlue.push_back({(float)config.distortionsBlue[i * 2], (float)config.distortionsBlue[i * 2 + 1]});
			}
		}
		newProfile = radialBezierProfile;
	}
	
	bool changed = false;
	
	if(newProfile != nullptr){
		// copy settings to new distortion profile
		newProfile->resolution = distortionSettings.resolution;
		// initialize new profile and replace old one
		newProfile->Initialize();
		if(profile != nullptr && profile != &distortionSettings){
			delete profile;
		}
		profile = newProfile;
		changed = true;
	}
	
	// fallback to default profile if nothing was set
	if(newProfile == nullptr && profile != &distortionSettings){
		if(profile != nullptr){
			delete profile;
		}
		profile = &distortionSettings;
		changed = true;
	}
	
	profileName = config.name;
	profileModifiedTime = config.modifiedTime;
	return changed;
}

DistortionProfileConstructor::~DistortionProfileConstructor(){
	if(profile != nullptr && profile != &distortionSettings){
		delete profile;
	}
}
