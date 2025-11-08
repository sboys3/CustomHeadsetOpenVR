#include "DistortionProfileConstructor.h"
#include "RadialBezierDistortionProfile.h"
#include <cmath>
#include <map>

std::unordered_map<std::string, DistortionProfileConfig> builtInDistortionProfiles = {};
// map old names to the new name of distortion profiles for comatability with old configs
std::unordered_map<std::string, std::string> builtInDistortionProfileAliases = {
	{"MeganeX8K Original", "MeganeX8K v0.3.0"}
};


bool PopulateBuiltInDistortionProfiles(){
	DistortionProfileConfig defaultProfile = {};
	defaultProfile.name = "MeganeX8K Default";
	defaultProfile.device = "MeganeX8K";
	defaultProfile.modifiedTime = 0;
	defaultProfile.description = "Default distortion profile for MeganeX8K. This is the profile made by Essentia that was called the Essentia Zoom Out profile. Previous profiles are too zoomed in when the diopters are set to zero, so this one provides more FOV. It works best if you can get close enough to the lenses.";
	defaultProfile.author = "Essentia, SBoys3, Shiftall";
	defaultProfile.creationDate = 1755911419.135;
	defaultProfile.type = "RadialBezier";
	defaultProfile.distortions = {
		 0.0000, 0.0,
		 5.0000, 12.024,
		10.0000, 23.904,
		15.0000, 35.429,
		20.0000, 46.511,
		25.0000, 57.162,
		30.0000, 67.498,
		35.0000, 77.596,
		40.0000, 86.882,
		45.0000, 94.532,
		48.3073, 98.599,
		50.0000, 100.265
	};
	builtInDistortionProfiles[defaultProfile.name] = defaultProfile;
	
	
	DistortionProfileConfig meganeX8Kv1_0_0 = {};
	meganeX8Kv1_0_0.name = "MeganeX8K v1.0.0";
	meganeX8Kv1_0_0.device = "MeganeX8K";
	meganeX8Kv1_0_0.modifiedTime = 0;
	meganeX8Kv1_0_0.description = "This is the profile made by Essentia that was called the Western B CA profile. It works pretty well if you can get close enough to be able to nearly see the 110 degree field of view.";
	meganeX8Kv1_0_0.author = "Essentia, SBoys3, Shiftall";
	meganeX8Kv1_0_0.creationDate = 1748554999.135;
	meganeX8Kv1_0_0.type = "RadialBezier";
	meganeX8Kv1_0_0.distortions = {
		0.00000f, 0.0f,
		5.00000f, 12.463f,
		10.0000f, 24.750f,
		15.00000f, 36.665f,
		20.00000f, 48.103f,
		25.00000f, 59.093f,
		30.00000f, 69.749f,
		35.00000f, 79.994f,
		40.00000f, 89.147f,
		45.00000f, 96.357f,
		48.30730f, 100.0f
	};
	builtInDistortionProfiles[meganeX8Kv1_0_0.name] = meganeX8Kv1_0_0;
	
	
	DistortionProfileConfig meganeX8Kv0_4_0 = {};
	meganeX8Kv0_4_0.name = "MeganeX8K v0.4.0";	
	meganeX8Kv0_4_0.modifiedTime = 0;
	meganeX8Kv0_4_0.description = "MeganeX8K distortion profile from CustomHeadset v0.4.0";
	meganeX8Kv0_4_0.author = "SBoys3, Shiftall";
	meganeX8Kv0_4_0.type = "RadialBezier";
	meganeX8Kv0_4_0.distortions = {
		0.00000f, 0.0f,
		10.0000f, 24.7f,
		20.0000f, 48.0f,
		30.0000f, 69.6f,
		35.0000f, 79.9f,
		40.0000f, 89.06f,
		45.0000f, 96.30f,
		48.3073f, 100.0f
	};
	builtInDistortionProfiles[meganeX8Kv0_4_0.name] = meganeX8Kv0_4_0;
	
	
	DistortionProfileConfig meganeX8Kv0_3_0 = {};
	meganeX8Kv0_3_0.name = "MeganeX8K v0.3.0";	
	meganeX8Kv0_3_0.modifiedTime = 0;
	meganeX8Kv0_3_0.description = "MeganeX8K original distortion profile which is the same as simplehmd.";
	meganeX8Kv0_3_0.author = "Shiftall";
	meganeX8Kv0_3_0.type = "RadialBezier";
	meganeX8Kv0_3_0.distortions = {
		00.0000f, 0.0f,
		10.0000f, 24.77952472f,
		20.0000f, 48.32328161f,
		30.0000f, 69.9136628f,
		35.0000f, 79.99462488f,
		40.0000f, 89.06057112f,
		45.0000f, 96.29634484f,
		48.3073f, 100.0f,
	};
	meganeX8Kv0_3_0.distortionsRed = {
		0.f, 0.5f,
		47.5f, 0.5f,
	};
	meganeX8Kv0_3_0.distortionsBlue = {
		0.f, -0.42f,
		47.5f, -0.42f,
	};
	builtInDistortionProfiles[meganeX8Kv0_3_0.name] = meganeX8Kv0_3_0;
	return true;
};
// this is pretty much just an initializer so immediately call
bool voidBool = PopulateBuiltInDistortionProfiles();


bool DistortionProfileConstructor::LoadDistortionProfile(std::string name){
	
	DistortionProfileConfig config = {};
	
	std::string aliasedName = name;
	if(builtInDistortionProfileAliases.find(name) != builtInDistortionProfileAliases.end()){
		aliasedName = builtInDistortionProfileAliases[name];
	}
	
	if(builtInDistortionProfiles.find(aliasedName) != builtInDistortionProfiles.end()){
		config = builtInDistortionProfiles[aliasedName];
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
		radialBezierProfile->smoothAmount = config.smoothAmount;
		newProfile = radialBezierProfile;
	}
	
	bool changed = false;
	
	if(newProfile != nullptr){
		if(profile != nullptr && profile != &distortionSettings){
			delete profile;
		}
		profile = newProfile;
		ReInitializeProfile();
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

void DistortionProfileConstructor::ReInitializeProfile(){
	// copy settings to new distortion profile
	profile->resolution = distortionSettings.resolution;
	profile->resolutionX = distortionSettings.resolutionX;
	profile->resolutionY = distortionSettings.resolutionY;
	profile->maxFovX = distortionSettings.maxFovX;
	profile->maxFovY = distortionSettings.maxFovY;
	profile->fovZoom = distortionSettings.fovZoom;
	if(profile->fovZoom == 0.0f){
		// avoid division by zero in calculations because invalid distortion data can prevent the compositor from starting
		profile->fovZoom = 1.0f; 
	}
	// initialize new profile and replace old one
	profile->Initialize();
}

void DistortionProfileConstructor::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight){
	uint32_t originalWidth = (uint32_t)distortionSettings.resolutionX;
	uint32_t originalHeight = (uint32_t)distortionSettings.resolutionY;
	uint32_t renderWidth = originalWidth;
	uint32_t renderHeight = originalHeight;
	if(profile != nullptr){
		profile->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
	}
	// keep total number of pixels the same but change aspect ratio
	double targetPixels = originalWidth * originalHeight;
	double renderPixels = renderWidth * renderHeight;
	renderWidth = (uint32_t)(renderWidth * std::sqrt(targetPixels / renderPixels));
	renderHeight = (uint32_t)(renderHeight * std::sqrt(targetPixels / renderPixels));
	renderWidth = std::min(renderWidth, 16384u);
	renderHeight = std::min(renderHeight, 16384u);
	DriverLog("100% render target size: %d x %d", renderWidth, renderHeight);
	*pnWidth = renderWidth;
	*pnHeight = renderHeight;
}

DistortionProfileConstructor::~DistortionProfileConstructor(){
	if(profile != nullptr && profile != &distortionSettings){
		delete profile;
	}
}
