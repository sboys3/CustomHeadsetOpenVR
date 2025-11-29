#include "ConfigLoader.h"
#include <map>
#include <thread>
#include <fstream>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "../Driver/DriverLog.h"
#include "../Distortion/DistortionProfileConstructor.h"
#include "Windows.h"



using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;


std::string ConfigLoader::GetConfigFolder(){
	char* appdataPath = std::getenv("APPDATA");
	std::string configPath = appdataPath == nullptr ? "./" : (std::string(appdataPath) + "/CustomHeadset/");
	return configPath;
}

void ConfigLoader::ParseConfig(){
	// acquire driverConfig.configLock for the duration of this function
	// std::lock_guard<std::mutex> lock(driverConfig.configLock);
	// get file at APPDATA/Roaming/CustomHeadset/settings.json
	std::string configPath = GetConfigFolder() + "settings.json";
	std::ifstream configFile(configPath);
	if(!configFile.is_open()){
		if(!hasLoggedConfigFileNotFound){
			hasLoggedConfigFileNotFound = true;
			DriverLog("Config file not found at %s, using default settings.", configPath.c_str());
		}
		return;
	}
	DriverLog("Loading config file from %s", configPath.c_str());
	try{
		// parse with support for comments
		json data = json::parse(configFile, nullptr, true, true);
		Config newConfig = {};
		if(data["meganeX8K"].is_object()){
			json meganeX8KData = data["meganeX8K"];
			if(meganeX8KData["enable"].is_boolean()){
				newConfig.meganeX8K.enable = meganeX8KData["enable"].get<bool>();
			}
			if(meganeX8KData["ipd"].is_number()){
				newConfig.meganeX8K.ipd = meganeX8KData["ipd"].get<double>();
			}
			if(meganeX8KData["ipdOffset"].is_number()){
				newConfig.meganeX8K.ipdOffset = meganeX8KData["ipdOffset"].get<double>();
			}
			if(meganeX8KData["blackLevel"].is_number()){
				newConfig.meganeX8K.blackLevel = meganeX8KData["blackLevel"].get<double>();
			}
			if(meganeX8KData["colorMultiplier"].is_object()){
				Config::Color &colorMultiplier = newConfig.meganeX8K.colorMultiplier;
				if(meganeX8KData["colorMultiplier"]["r"].is_number()){ colorMultiplier.r = meganeX8KData["colorMultiplier"]["r"].get<double>(); }
				if(meganeX8KData["colorMultiplier"]["g"].is_number()){ colorMultiplier.g = meganeX8KData["colorMultiplier"]["g"].get<double>(); }
				if(meganeX8KData["colorMultiplier"]["b"].is_number()){ colorMultiplier.b = meganeX8KData["colorMultiplier"]["b"].get<double>(); }
			}
			if(meganeX8KData["distortionProfile"].is_string()){
				newConfig.meganeX8K.distortionProfile = meganeX8KData["distortionProfile"].get<std::string>();
			}
			if(meganeX8KData["distortionZoom"].is_number()){
				newConfig.meganeX8K.distortionZoom = meganeX8KData["distortionZoom"].get<double>();
			}
			if(meganeX8KData["fovZoom"].is_number()){
				newConfig.meganeX8K.fovZoom = meganeX8KData["fovZoom"].get<double>();
			}
			if(meganeX8KData["subpixelShift"].is_number()){
				newConfig.meganeX8K.subpixelShift = meganeX8KData["subpixelShift"].get<double>();
			}
			if(meganeX8KData["resolutionX"].is_number()){
				newConfig.meganeX8K.resolutionX = meganeX8KData["resolutionX"].get<int>();
			}
			if(meganeX8KData["resolutionY"].is_number()){
				newConfig.meganeX8K.resolutionY = meganeX8KData["resolutionY"].get<int>();
			}
			if(meganeX8KData["maxFovX"].is_number()){
				newConfig.meganeX8K.maxFovX = meganeX8KData["maxFovX"].get<double>();
			}
			if(meganeX8KData["maxFovY"].is_number()){
				newConfig.meganeX8K.maxFovY = meganeX8KData["maxFovY"].get<double>();
			}
			if(meganeX8KData["distortionMeshResolution"].is_number()){
				newConfig.meganeX8K.distortionMeshResolution = meganeX8KData["distortionMeshResolution"].get<int>();
			}
			if(meganeX8KData["fovBurnInPrevention"].is_boolean()){
				newConfig.meganeX8K.fovBurnInPrevention = meganeX8KData["fovBurnInPrevention"].get<bool>();
			}
			if(meganeX8KData["renderResolutionMultiplierX"].is_number()){
				newConfig.meganeX8K.renderResolutionMultiplierX = meganeX8KData["renderResolutionMultiplierX"].get<double>();
			}
			if(meganeX8KData["renderResolutionMultiplierY"].is_number()){
				newConfig.meganeX8K.renderResolutionMultiplierY = meganeX8KData["renderResolutionMultiplierY"].get<double>();
			}
			if(meganeX8KData["superSamplingFilterPercent"].is_number()){
				newConfig.meganeX8K.superSamplingFilterPercent = meganeX8KData["superSamplingFilterPercent"].get<double>();
			}
			if(meganeX8KData["secondsFromVsyncToPhotons"].is_number()){
				newConfig.meganeX8K.secondsFromVsyncToPhotons = meganeX8KData["secondsFromVsyncToPhotons"].get<double>();
			}
			if(meganeX8KData["secondsFromPhotonsToVblank"].is_number()){
				newConfig.meganeX8K.secondsFromPhotonsToVblank = meganeX8KData["secondsFromPhotonsToVblank"].get<double>();
			}
			if(meganeX8KData["eyeRotation"].is_number()){
				newConfig.meganeX8K.eyeRotation = meganeX8KData["eyeRotation"].get<double>();
			}
			if(meganeX8KData["disableEye"].is_number()){
				newConfig.meganeX8K.disableEye = meganeX8KData["disableEye"].get<int>();
			}
			if(meganeX8KData["disableEyeDecreaseFov"].is_boolean()){
				newConfig.meganeX8K.disableEyeDecreaseFov = meganeX8KData["disableEyeDecreaseFov"].get<bool>();
			}
			if(meganeX8KData["bluetoothDevice"].is_number()){
				newConfig.meganeX8K.bluetoothDevice = meganeX8KData["bluetoothDevice"].get<int>();
			}
			if(meganeX8KData["directMode"].is_boolean()){
				newConfig.meganeX8K.directMode = meganeX8KData["directMode"].get<bool>();
			}
			
			if(json& hiddenAreaJson = meganeX8KData["hiddenArea"]; hiddenAreaJson.is_object()){
				auto& newHiddenArea = newConfig.meganeX8K.hiddenArea;
				if(hiddenAreaJson["enable"].is_boolean()){ newHiddenArea.enable = hiddenAreaJson["enable"].get<bool>(); }
				if(hiddenAreaJson["testMode"].is_boolean()){ newHiddenArea.testMode = hiddenAreaJson["testMode"].get<bool>(); }
				if(hiddenAreaJson["detailLevel"].is_number()){ newHiddenArea.detailLevel = hiddenAreaJson["detailLevel"].get<int>(); }
				if(hiddenAreaJson["radiusTopOuter"].is_number()){ newHiddenArea.radiusTopOuter = hiddenAreaJson["radiusTopOuter"].get<double>(); }
				if(hiddenAreaJson["radiusTopInner"].is_number()){ newHiddenArea.radiusTopInner = hiddenAreaJson["radiusTopInner"].get<double>(); }
				if(hiddenAreaJson["radiusBottomInner"].is_number()){ newHiddenArea.radiusBottomInner = hiddenAreaJson["radiusBottomInner"].get<double>(); }
				if(hiddenAreaJson["radiusBottomOuter"].is_number()){ newHiddenArea.radiusBottomOuter = hiddenAreaJson["radiusBottomOuter"].get<double>(); }
			}
			if(json& stationaryDimmingJson = meganeX8KData["stationaryDimming"]; stationaryDimmingJson.is_object()){
				auto& newStationaryDimming = newConfig.meganeX8K.stationaryDimming;
				if(stationaryDimmingJson["enable"].is_boolean()){ newStationaryDimming.enable = stationaryDimmingJson["enable"].get<bool>(); }
				if(stationaryDimmingJson["movementThreshold"].is_number()){ newStationaryDimming.movementThreshold = stationaryDimmingJson["movementThreshold"].get<double>(); }
				if(stationaryDimmingJson["movementTime"].is_number()){ newStationaryDimming.movementTime = stationaryDimmingJson["movementTime"].get<double>(); }
				if(stationaryDimmingJson["dimBrightnessPercent"].is_number()){ newStationaryDimming.dimBrightnessPercent = stationaryDimmingJson["dimBrightnessPercent"].get<double>(); }
				if(stationaryDimmingJson["dimSeconds"].is_number()){ newStationaryDimming.dimSeconds = stationaryDimmingJson["dimSeconds"].get<double>(); }
				if(stationaryDimmingJson["brightenSeconds"].is_number()){ newStationaryDimming.brightenSeconds = stationaryDimmingJson["brightenSeconds"].get<double>(); }
			}
		}
		if(data["customShader"].is_object()){
			json customShaderData = data["customShader"];
			if(customShaderData["enable"].is_boolean()){
				newConfig.customShader.enable = customShaderData["enable"].get<bool>();
			}
			if(customShaderData["enableForMeganeX8K"].is_boolean()){
				newConfig.customShader.enableForMeganeX8K = customShaderData["enableForMeganeX8K"].get<bool>();
			}
			if(customShaderData["enableForOther"].is_boolean()){
				newConfig.customShader.enableForOther = customShaderData["enableForOther"].get<bool>();
			}
			if(customShaderData["contrast"].is_number()){
				newConfig.customShader.contrast = customShaderData["contrast"].get<double>();
			}
			if(customShaderData["contrastMidpoint"].is_number()){
				newConfig.customShader.contrastMidpoint = customShaderData["contrastMidpoint"].get<double>();
			}
			if(customShaderData["contrastLinear"].is_boolean()){
				newConfig.customShader.contrastLinear = customShaderData["contrastLinear"].get<bool>();
			}
			if(customShaderData["contrastPerEye"].is_boolean()){
				newConfig.customShader.contrastPerEye = customShaderData["contrastPerEye"].get<bool>();
			}
			if(customShaderData["contrastPerEyeLinear"].is_boolean()){
				newConfig.customShader.contrastPerEyeLinear = customShaderData["contrastPerEyeLinear"].get<bool>();
			}
			if(customShaderData["contrastLeft"].is_number()){
				newConfig.customShader.contrastLeft = customShaderData["contrastLeft"].get<double>();
			}
			if(customShaderData["contrastMidpointLeft"].is_number()){
				newConfig.customShader.contrastMidpointLeft = customShaderData["contrastMidpointLeft"].get<double>();
			}
			if(customShaderData["contrastRight"].is_number()){
				newConfig.customShader.contrastRight = customShaderData["contrastRight"].get<double>();
			}
			if(customShaderData["contrastMidpointRight"].is_number()){
				newConfig.customShader.contrastMidpointRight = customShaderData["contrastMidpointRight"].get<double>();
			}
			if(customShaderData["chroma"].is_number()){
				newConfig.customShader.chroma = customShaderData["chroma"].get<double>();
			}
			if(customShaderData["gamma"].is_number()){
				newConfig.customShader.gamma = customShaderData["gamma"].get<double>();
			}
			if(customShaderData["subpixelShift"].is_boolean()){
				newConfig.customShader.subpixelShift = customShaderData["subpixelShift"].get<bool>();
			}
			if(customShaderData["disableMuraCorrection"].is_boolean()){
				newConfig.customShader.disableMuraCorrection = customShaderData["disableMuraCorrection"].get<bool>();
			}
			if(customShaderData["disableBlackLevels"].is_boolean()){
				newConfig.customShader.disableBlackLevels = customShaderData["disableBlackLevels"].get<bool>();
			}
			if(customShaderData["srgbColorCorrection"].is_boolean()){
				newConfig.customShader.srgbColorCorrection = customShaderData["srgbColorCorrection"].get<bool>();
			}
			if(customShaderData["srgbColorCorrectionMatrix"].is_array()){
				newConfig.customShader.srgbColorCorrectionMatrix = customShaderData["srgbColorCorrectionMatrix"].get<std::vector<double>>();
			}
			if(customShaderData["lensColorCorrection"].is_boolean()){
				newConfig.customShader.lensColorCorrection = customShaderData["lensColorCorrection"].get<bool>();
			}
			if(customShaderData["dither10Bit"].is_boolean()){
				newConfig.customShader.dither10Bit = customShaderData["dither10Bit"].get<bool>();
			}
		}
		if(data["forceTracking"].is_boolean()){
			newConfig.forceTracking = data["forceTracking"].get<bool>();
		}
		// if(data["watchDistortionProfiles"].is_boolean()){
		// 	newConfig.watchDistortionProfiles = data["watchDistortionProfiles"].get<bool>();
		// }
		// write to global config
		{
			std::lock_guard<std::mutex> lock(driverConfigLock);
			driverConfigOld = driverConfig;
			driverConfig = newConfig;
		}
	}catch(const std::exception& e){
		DriverLog("Failed to parse config file: %s", e.what());
		return;
	}
}

DistortionProfileConfig ConfigLoader::ParseDistortionConfig(std::string name){
	std::string profilePath = GetConfigFolder() + "Distortion/" + name + ".json";
	std::ifstream configFile(profilePath);
	if(!configFile.is_open()){
		DriverLog("Distortion profile not found at %s", profilePath.c_str());
		return {};
	}
	DriverLog("Loading distortion profile from %s", profilePath.c_str());
	try{
		// parse with support for comments
		json data = json::parse(configFile, nullptr, true, true);
		DistortionProfileConfig profile = {};
		profile.modifiedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::filesystem::last_write_time(profilePath).time_since_epoch()).count() / 1000000000.0;
		profile.name = name;
		if(data["description"].is_string()){
			profile.description = data["description"].get<std::string>();
		}
		if(data["type"].is_string()){
			profile.type = data["type"].get<std::string>();
		}
		if(data["distortions"].is_array()){
			profile.distortions = data["distortions"].get<std::vector<double>>();
		}
		if(data["distortionsRed"].is_array()){
			profile.distortionsRed = data["distortionsRed"].get<std::vector<double>>();
		}
		if(data["distortionsBlue"].is_array()){
			profile.distortionsBlue = data["distortionsBlue"].get<std::vector<double>>();
		}
		if(data["smoothAmount"].is_number()){
			profile.smoothAmount = data["smoothAmount"].get<double>();
		}
		return profile;
	}catch(const std::exception& e){
		DriverLog("Failed to parse distortion profile: %s", e.what());
		return {};
	}
}

void ConfigLoader::WriteInfo(){
	std::string infoPath = GetConfigFolder() + "info.json";
	std::ofstream infoFile(infoPath);
	if(!infoFile.is_open()){
		DriverLog("Failed to open info.json for writing: %d", GetLastError());
		return;
	}
	Config defaultSettings = {};
	std::map<std::string,int> emptyObject = {};
	ordered_json data = {
		{"about", "This file is not for configuration. It provides info from the driver for other utilities to use."},
		{"defaultSettings", {
			{"meganeX8K", {
				{"enable", defaultSettings.meganeX8K.enable},
				{"ipd", defaultSettings.meganeX8K.ipd},
				{"ipdOffset", defaultSettings.meganeX8K.ipdOffset},
				{"blackLevel", defaultSettings.meganeX8K.blackLevel},
				{"colorMultiplier", {
					{"r", defaultSettings.meganeX8K.colorMultiplier.r},
					{"g", defaultSettings.meganeX8K.colorMultiplier.g},
					{"b", defaultSettings.meganeX8K.colorMultiplier.b},
				}},
				{"distortionProfile", defaultSettings.meganeX8K.distortionProfile},
				{"distortionZoom", defaultSettings.meganeX8K.distortionZoom},
				{"fovZoom", defaultSettings.meganeX8K.fovZoom},
				{"subpixelShift", defaultSettings.meganeX8K.subpixelShift},
				{"resolutionX", defaultSettings.meganeX8K.resolutionX},
				{"resolutionY", defaultSettings.meganeX8K.resolutionY},
				{"maxFovX", defaultSettings.meganeX8K.maxFovX},
				{"maxFovY", defaultSettings.meganeX8K.maxFovY},
				{"distortionMeshResolution", defaultSettings.meganeX8K.distortionMeshResolution},
				{"fovBurnInPrevention", defaultSettings.meganeX8K.fovBurnInPrevention},
				{"renderResolutionMultiplierX", defaultSettings.meganeX8K.renderResolutionMultiplierX},
				{"renderResolutionMultiplierY", defaultSettings.meganeX8K.renderResolutionMultiplierY},
				{"superSamplingFilterPercent", defaultSettings.meganeX8K.superSamplingFilterPercent},
				{"secondsFromVsyncToPhotons", defaultSettings.meganeX8K.secondsFromVsyncToPhotons},
				{"secondsFromPhotonsToVblank", defaultSettings.meganeX8K.secondsFromPhotonsToVblank},
				{"eyeRotation", defaultSettings.meganeX8K.eyeRotation},
				{"disableEye", defaultSettings.meganeX8K.disableEye},
				{"disableEyeDecreaseFov", defaultSettings.meganeX8K.disableEyeDecreaseFov},
				{"bluetoothDevice", defaultSettings.meganeX8K.bluetoothDevice},
				{"directMode", defaultSettings.meganeX8K.directMode},
				{"hiddenArea", {
					{"enable", defaultSettings.meganeX8K.hiddenArea.enable},
					{"testMode", defaultSettings.meganeX8K.hiddenArea.testMode},
					{"detailLevel", defaultSettings.meganeX8K.hiddenArea.detailLevel},
					{"radiusTopOuter", defaultSettings.meganeX8K.hiddenArea.radiusTopOuter},
					{"radiusTopInner", defaultSettings.meganeX8K.hiddenArea.radiusTopInner},
					{"radiusBottomInner", defaultSettings.meganeX8K.hiddenArea.radiusBottomInner},
					{"radiusBottomOuter", defaultSettings.meganeX8K.hiddenArea.radiusBottomOuter},
				}},
				{"stationaryDimming", {
					{"enable", defaultSettings.meganeX8K.stationaryDimming.enable},
					{"movementThreshold", defaultSettings.meganeX8K.stationaryDimming.movementThreshold},
					{"movementTime", defaultSettings.meganeX8K.stationaryDimming.movementTime},
					{"dimBrightnessPercent", defaultSettings.meganeX8K.stationaryDimming.dimBrightnessPercent},
					{"dimSeconds", defaultSettings.meganeX8K.stationaryDimming.dimSeconds},
					{"brightenSeconds", defaultSettings.meganeX8K.stationaryDimming.brightenSeconds},
				}},
			}},
			{"customShader", {
				{"enable", defaultSettings.customShader.enable},
				{"enableForMeganeX8K", defaultSettings.customShader.enableForMeganeX8K},
				{"enableForOther", defaultSettings.customShader.enableForOther},
				{"contrast", defaultSettings.customShader.contrast},
				{"contrastMidpoint", defaultSettings.customShader.contrastMidpoint},
				{"contrastLinear", defaultSettings.customShader.contrastLinear},
				{"contrastPerEye", defaultSettings.customShader.contrastPerEye},
				{"contrastPerEyeLinear", defaultSettings.customShader.contrastPerEyeLinear},
				{"contrastLeft", defaultSettings.customShader.contrastLeft},
				{"contrastMidpointLeft", defaultSettings.customShader.contrastMidpointLeft},
				{"contrastRight", defaultSettings.customShader.contrastRight},
				{"contrastMidpointRight", defaultSettings.customShader.contrastMidpointRight},
				{"chroma", defaultSettings.customShader.chroma},
				{"gamma", defaultSettings.customShader.gamma},
				{"subpixelShift", defaultSettings.customShader.subpixelShift},
				{"disableMuraCorrection", defaultSettings.customShader.disableMuraCorrection},
				{"disableBlackLevels", defaultSettings.customShader.disableBlackLevels},
				{"srgbColorCorrection", defaultSettings.customShader.srgbColorCorrection},
				{"srgbColorCorrectionMatrix", defaultSettings.customShader.srgbColorCorrectionMatrix},
				{"lensColorCorrection", defaultSettings.customShader.lensColorCorrection},
				{"dither10Bit", defaultSettings.customShader.dither10Bit},
			}},
			{"forceTracking", defaultSettings.forceTracking},
			// {"watchDistortionProfiles", defaultSettings.watchDistortionProfiles}
		}},
		{"builtInDistortionProfiles", emptyObject},
		{"resolution", {
			{"fovX", info.renderFovX},
			{"fovY", info.renderFovY},
			{"fovMaxX", info.renderFovMaxX},
			{"fovMaxY", info.renderFovMaxY},
			{"renderResolution1To1X", info.renderResolution1To1X},
			{"renderResolution1To1Y", info.renderResolution1To1Y},
			{"renderResolution1To1Percent", info.renderResolution1To1Percent},
			{"renderResolution100PercentX", info.renderResolution100PercentX},
			{"renderResolution100PercentY", info.renderResolution100PercentY},
		}},
		{"connectedHeadset", (int)info.connectedHeadset},
		{"debugLog", info.debugLog},
		{"driverResources", info.driverResources},
		{"steamvrResources", info.steamvrResources},
		{"driverVersion", driverVersion}
	};
	ordered_json& distortionProfilesJson = data["builtInDistortionProfiles"];
	for(auto profilePair : builtInDistortionProfiles){
		auto profile = profilePair.second;
		ordered_json profileJson = {
			{"device", profile.device},
			{"description", profile.description},
			{"author", profile.author},
			{"creationDate", profile.creationDate},
			{"type", profile.type},
			{"distortions", profile.distortions},
			{"distortionsRed", profile.distortionsRed},
			{"distortionsBlue", profile.distortionsBlue},
			{"smoothAmount", profile.smoothAmount},
		};
		distortionProfilesJson[profile.name] = profileJson;
	}
	infoFile << data.dump(1, '\t');
	infoFile.close();
}

void ConfigLoader::ReadInfo(){
	std::string infoPath = GetConfigFolder() + "info.json";
	std::ifstream infoFile(infoPath);
	if(!infoFile.is_open()){
		DriverLog("Failed to open info.json for reading: %d", GetLastError());
		return;
	}
	try{
		std::lock_guard<std::mutex> lock(driverConfigLock);
		json data = json::parse(infoFile, nullptr, true, true);
		if(data["driverResources"].is_string()){
			info.driverResources = data["driverResources"].get<std::string>();
		}
		if(data["steamvrResources"].is_string()){
			info.steamvrResources = data["steamvrResources"].get<std::string>();
		}
		if(data["connectedHeadset"].is_number()){
			info.connectedHeadset = (HeadsetType)data["connectedHeadset"].get<int>();
		}
		info.hasBeenUpdated = true; 
	}catch(const std::exception& e){
		DriverLog("Failed to parse info.json: %s", e.what());
		return;
	}
}

// void ConfigLoader::WriteInfoThread(){
	// while(started){
	// 	WriteInfo();
	// 	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	// }
// }

void ConfigLoader::WatcherThread(){
	// watch for changes in the config file directory
	std::string configPath = GetConfigFolder();
	HANDLE hDir = CreateFileA(configPath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(hDir == INVALID_HANDLE_VALUE){
		DriverLog("Failed to open config directory for watching: %d", GetLastError());
		return;
	}
	while(started){
		DWORD bytesReturned;
		char buffer[1024]{};
		FILE_NOTIFY_INFORMATION* pNotify;
		BOOL success = ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, NULL, NULL);
		if(!success){
			DriverLog("Failed to read directory changes: %d", GetLastError());
			break;
		}
		pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
		bool hasReloadedConfig = false;
		bool hasReloadedInfo = false;
		do{
			std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t));
			if(!hasReloadedConfig && fileName == L"settings.json" && (pNotify->Action == FILE_ACTION_MODIFIED || pNotify->Action == FILE_ACTION_ADDED || pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME)){
				DriverLog("Config file changed, reloading...");
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				while(driverConfig.hasBeenUpdated){
					// wait for the last config update to be used before doing another one
					std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
				}
				ParseConfig();
				hasReloadedConfig = true;
			}
			if(watchInfo && !hasReloadedInfo && fileName == L"info.json" && (pNotify->Action == FILE_ACTION_MODIFIED || pNotify->Action == FILE_ACTION_ADDED || pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME)){
				DriverLog("Info file changed, reloading...");
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				ReadInfo();
				hasReloadedInfo = true;
			}
			pNotify = (FILE_NOTIFY_INFORMATION*)((char*)pNotify + pNotify->NextEntryOffset);
		}while(pNotify->NextEntryOffset != 0);
		//DriverLog("Waiting for next change...");
		std::this_thread::sleep_for(std::chrono::milliseconds(40));
	}
}

void ConfigLoader::WatcherThreadDistortions(){
	std::string configPath = GetConfigFolder() + "Distortion/";
	HANDLE hDir = CreateFileA(configPath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(hDir == INVALID_HANDLE_VALUE){
		DriverLog("Failed to open distortion directory for watching: %d", GetLastError());
		return;
	}
	while(started){
		DWORD bytesReturned;
		char buffer[1024]{};
		FILE_NOTIFY_INFORMATION* pNotify;
		BOOL success = ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, NULL, NULL);
		if(!success){
			DriverLog("Failed to read directory changes: %d", GetLastError());
			break;
		}
		pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
		do{
			std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t));
			if(fileName.find(L".json") != std::wstring::npos && (pNotify->Action == FILE_ACTION_MODIFIED || pNotify->Action == FILE_ACTION_ADDED || pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME)){
				DriverLog("Distortion profile changed, reloading...");
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				while(driverConfig.hasBeenUpdated){
					// wait for the last config update to be used before doing another one
					std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
				}
				ParseConfig();
				break;
			}
			pNotify = (FILE_NOTIFY_INFORMATION*)((char*)pNotify + pNotify->NextEntryOffset);
		}while(pNotify->NextEntryOffset != 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(40));
	}
}
				
// only define settings that most users will change and are unlikely to have their default changed
// settings not defined here will easily be able to have their defaults changed in the future for everyone
std::string defaultConfig = R"({
	"meganeX8K": {
		"enable": true,
		"ipd": 63.0,
		"ipdOffset": 0.0,
		"distortionProfile": "MeganeX8K Default"
	}
})";


void ConfigLoader::Start(){
	if(started){
		return;
	}
	started = true;
	
	try{
		// create directory
		std::filesystem::create_directories(GetConfigFolder());
		
		// create default config if it doesn't exist
		std::string configPath = GetConfigFolder() + "settings.json";
		if(!std::filesystem::exists(configPath)){
			std::ofstream configFile(configPath);
			configFile << defaultConfig;
			configFile.close();
		}
		
		if(watchInfo){
			ReadInfo();
		}else{
			WriteInfo();
			// start info thread
			// std::thread infoThread(&ConfigLoader::WriteInfoThread, this);
			// infoThread.detach();
		}
	}catch(const std::exception& e){
		DriverLog("Failed to create settings.json %s", e.what());
	}
	
	// load config for the first time
	ParseConfig();
	
	try{	
		// start watcher thread
		std::thread watcher(&ConfigLoader::WatcherThread, this);
		// detach the thread to run forever
		watcher.detach();
		
		// create distortion profiles directory and watch if configured
		std::filesystem::create_directories(GetConfigFolder() + "Distortion/");
		// if(driverConfig.watchDistortionProfiles){
		std::thread distortionWatcher(&ConfigLoader::WatcherThreadDistortions, this);
		distortionWatcher.detach();
		// }
	}catch(const std::exception& e){
		DriverLog("Failed to start config watcher: %s", e.what());
	}
}


ConfigLoader driverConfigLoader = {};