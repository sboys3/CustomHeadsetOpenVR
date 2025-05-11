#pragma once
#include "../Config/ConfigLoader.h"
#include "DistortionProfile.h"
#include "NoneDistortionProfile.h"

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
		virtual ~DistortionProfileConstructor();
	private:
		std::string profileName;
		double profileModifiedTime;
};