#include "DriverLockout.h"
#include "../Driver/DriverLog.h"
#include "../Config/Config.h"
#include <fstream>
#include <sstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::string GetSteamVRConfigPath(){
	// Read openvrpaths.vrpath directly, same as the GUI does
	// On Windows: %LOCALAPPDATA%\openvr\openvrpaths.vrpath
	// On Linux: ~/.local/share/openvr/openvrpaths.vrpath
	std::string openvrPathsFile;
	
	#ifdef _WIN32
	char* localAppData = std::getenv("LOCALAPPDATA");
	if(localAppData){
		openvrPathsFile = std::string(localAppData) + "\\openvr\\openvrpaths.vrpath";
	}
	#else
	char* home = std::getenv("HOME");
	if(home){
		openvrPathsFile = std::string(home) + "/.local/share/openvr/openvrpaths.vrpath";
	}
	#endif
	
	if(openvrPathsFile.empty()){
		return "";
	}
	
	std::ifstream file(openvrPathsFile);
	if(!file.is_open()){
		DriverLog("DriverLockout: Could not open openvrpaths.vrpath at %s", openvrPathsFile.c_str());
		return "";
	}
	
	try{
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string content = buffer.str();
		file.close();
		
		json openvrPaths = json::parse(content);
		
		// Get the config path array (first element)
		if(openvrPaths.contains("config")){
			json configArray = openvrPaths["config"];
			if(configArray.is_array() && !configArray.empty()){
				std::string configPath = configArray[0].get<std::string>();
				if(!configPath.empty()){
					// Ensure path ends with separator
					#ifdef _WIN32
					if(configPath.back() != '\\'){
						configPath += '\\';
					}
					#else
					if(configPath.back() != '/'){
						configPath += '/';
					}
					#endif
					return configPath;
				}
			}
		}
		
		DriverLog("DriverLockout: No config path found in openvrpaths.vrpath");
	}catch(const std::exception& e){
		DriverLog("DriverLockout: Failed to parse openvrpaths.vrpath: %s", e.what());
	}
	
	return "";
}

bool IsDriverEnabled(const char* driverName){
	std::string configPath = GetSteamVRConfigPath();
	if(configPath.empty()){
		// If we can't find the config path, treat as disabled (not present = disabled per requirements)
		return false;
	}
	
	std::string settingsFile = configPath + "steamvr.vrsettings";
	DriverLog("DriverLockout: Checking SteamVR settings at %s", settingsFile.c_str());
	std::ifstream file(settingsFile);
	if(!file.is_open()){
		// If the settings file doesn't exist at all, treat as disabled per requirements
		return false;
	}
	
	try{
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string content = buffer.str();
		file.close();
		
		json settings = json::parse(content);
		
		std::string driverKey = "driver_" + std::string(driverName);
		if(!settings.contains(driverKey)){
			// Driver not present in settings at all - treat as disabled per requirements
			return false;
		}
		
		json driverSetting = settings[driverKey];
		
		// Check if blocked by safe mode
		if(driverSetting.contains("blocked_by_safe_mode")){
			bool blocked = driverSetting["blocked_by_safe_mode"].get<bool>();
			if(blocked){
				return false;
			}
		}
		
		// Check enable state (default to true if not specified)
		if(driverSetting.contains("enable")){
			bool enabled = driverSetting["enable"].get<bool>();
			return enabled;
		}
		
		// Driver present but enable not specified - treat as enabled
		return true;
	}catch(const std::exception& e){
		DriverLog("DriverLockout: Failed to parse steamvr.vrsettings: %s", e.what());
		// On parse error, treat as disabled to be safe
		return false;
	}
}

bool IsNeutralDriverEnabled(){
	return IsDriverEnabled("CustomHeadsetOpenVR");
}

bool WriteHasBeenRunSetting(const char* driverName){
	std::string section = "driver_" + std::string(driverName);
	vr::EVRSettingsError error;
	vr::VRSettings()->SetString(section.c_str(), "hasBeenRun", driverVersion.c_str(), &error);
	if(error != vr::VRSettingsError_None){
		DriverLog("DriverLockout: Failed to write hasBeenRun for %s, error: %s", driverName, vr::VRSettings()->GetSettingsErrorNameFromEnum(error));
		return false;
	}
	DriverLog("DriverLockout: Successfully wrote hasBeenRun = %s for %s", driverVersion.c_str(), driverName);
	return true;
}