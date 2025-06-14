#pragma once
#include "../Config/ConfigLoader.h"
#include "DistortionProfile.h"
#include "NoneDistortionProfile.h"
#include <unordered_map>

// this class is responsible for loading distortion profiles based on names
class DistortionProfileConstructor{
	public:
		// this must be filled with all the initial settings
		// the settings will be transferred to profiles on load
		// this also serves as a fallback default for profile when one can not be loaded
		NoneDistortionProfile distortionSettings;
		// current distortion profile
		DistortionProfile* profile = &distortionSettings;
		// load a distortion profile by name
		// returns true if the profile was changed to indicate the distortion mesh must be refreshed
		bool LoadDistortionProfile(std::string name);
		// re-initialize the existing profile with the new distortionSettings
		void ReInitializeProfile();
		// get the 100% size for the distortion profile
		// this will keep the total number of pixels similar to the actual panel resolution but will change the aspect ratio to match the distortion profile
		void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight);
		virtual ~DistortionProfileConstructor();
	private:
		// used for detecting profile changes
		std::string profileName = "";
		double profileModifiedTime = 0.0;
};
extern std::unordered_map<std::string, DistortionProfileConfig> builtInDistortionProfiles;