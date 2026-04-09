#include "ConfigLoader.h"
#include <map>
#include <thread>
#include <fstream>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "../Driver/DriverLog.h"
#include "../Distortion/DistortionProfileConstructor.h"
#ifdef _WIN32
#include "Windows.h"
#endif

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;


std::string ConfigLoader::GetConfigFolder(){
	#ifdef _WIN32
	char* appdataPath = std::getenv("APPDATA");
	std::string configPath = appdataPath == nullptr ? "./" : (std::string(appdataPath) + "/CustomHeadset/");
	#elif __linux__
	char* appdataPath = std::getenv("HOME");
	std::string configPath = appdataPath == nullptr ? "./" : (std::string(appdataPath) + "/.config/CustomHeadset/");
	#endif
	return configPath;
}

void parseBaseHeadsetConfig(json headsetData, Config::BaseHeadsetConfig& headsetConfig){
	if(headsetData["enable"].is_boolean()){
		headsetConfig.enable = headsetData["enable"].get<bool>();
	}
	if(headsetData["ipd"].is_number()){
		headsetConfig.ipd = headsetData["ipd"].get<double>();
	}
	if(headsetData["ipdOffset"].is_number()){
		headsetConfig.ipdOffset = headsetData["ipdOffset"].get<double>();
	}
	if(headsetData["blackLevel"].is_number()){
		headsetConfig.blackLevel = headsetData["blackLevel"].get<double>();
	}
	if(headsetData["colorMultiplier"].is_object()){
		ConfigColor &colorMultiplier = headsetConfig.colorMultiplier;
		if(headsetData["colorMultiplier"]["r"].is_number()){ colorMultiplier.r = headsetData["colorMultiplier"]["r"].get<double>(); }
		if(headsetData["colorMultiplier"]["g"].is_number()){ colorMultiplier.g = headsetData["colorMultiplier"]["g"].get<double>(); }
		if(headsetData["colorMultiplier"]["b"].is_number()){ colorMultiplier.b = headsetData["colorMultiplier"]["b"].get<double>(); }
	}
	if(headsetData["distortionProfile"].is_string()){
		headsetConfig.distortionProfile = headsetData["distortionProfile"].get<std::string>();
	}
	if(headsetData["distortionZoom"].is_number()){
		headsetConfig.distortionZoom = headsetData["distortionZoom"].get<double>();
	}
	if(headsetData["fovZoom"].is_number()){
		headsetConfig.fovZoom = headsetData["fovZoom"].get<double>();
	}
	if(headsetData["subpixelShift"].is_number()){
		headsetConfig.subpixelShift = headsetData["subpixelShift"].get<double>();
	}
	if(headsetData["resolutionX"].is_number()){
		headsetConfig.resolutionX = headsetData["resolutionX"].get<int>();
	}
	if(headsetData["resolutionY"].is_number()){
		headsetConfig.resolutionY = headsetData["resolutionY"].get<int>();
	}
	if(headsetData["displayRotation"].is_number()){
		headsetConfig.displayRotation = headsetData["displayRotation"].get<int>();
	}
	if(headsetData["maxFovX"].is_number()){
		headsetConfig.maxFovX = headsetData["maxFovX"].get<double>();
	}
	if(headsetData["maxFovY"].is_number()){
		headsetConfig.maxFovY = headsetData["maxFovY"].get<double>();
	}
	if(headsetData["distortionMeshResolution"].is_number()){
		headsetConfig.distortionMeshResolution = headsetData["distortionMeshResolution"].get<int>();
	}
	if(headsetData["fovBurnInPrevention"].is_boolean()){
		headsetConfig.fovBurnInPrevention = headsetData["fovBurnInPrevention"].get<bool>();
	}
	if(headsetData["renderResolutionMultiplierX"].is_number()){
		headsetConfig.renderResolutionMultiplierX = headsetData["renderResolutionMultiplierX"].get<double>();
	}
	if(headsetData["renderResolutionMultiplierY"].is_number()){
		headsetConfig.renderResolutionMultiplierY = headsetData["renderResolutionMultiplierY"].get<double>();
	}
	if(headsetData["superSamplingFilterPercent"].is_number()){
		headsetConfig.superSamplingFilterPercent = headsetData["superSamplingFilterPercent"].get<double>();
	}
	if(headsetData["secondsFromVsyncToPhotons"].is_number()){
		headsetConfig.secondsFromVsyncToPhotons = headsetData["secondsFromVsyncToPhotons"].get<double>();
	}
	if(headsetData["secondsFromPhotonsToVblank"].is_number()){
		headsetConfig.secondsFromPhotonsToVblank = headsetData["secondsFromPhotonsToVblank"].get<double>();
	}
	if(headsetData["eyeRotation"].is_number()){
		headsetConfig.eyeRotation = headsetData["eyeRotation"].get<double>();
	}
	if(headsetData["disableEye"].is_number()){
		headsetConfig.disableEye = headsetData["disableEye"].get<int>();
	}
	if(headsetData["disableEyeDecreaseFov"].is_boolean()){
		headsetConfig.disableEyeDecreaseFov = headsetData["disableEyeDecreaseFov"].get<bool>();
	}
	if(headsetData["useViveBluetooth"].is_boolean()){
		headsetConfig.useViveBluetooth = headsetData["useViveBluetooth"].get<bool>();
	}
	if(headsetData["directMode"].is_boolean()){
		headsetConfig.directMode = headsetData["directMode"].get<bool>();
	}
	if(headsetData["replaceIcons"].is_boolean()){
		headsetConfig.replaceIcons = headsetData["replaceIcons"].get<bool>();
	}
	if(headsetData["edidVendorIdOverride"].is_number()){
		headsetConfig.edidVendorIdOverride = headsetData["edidVendorIdOverride"].get<int>();
	}
	if(headsetData["edidProductIdOverride"].is_number()){
		headsetConfig.edidProductIdOverride = headsetData["edidProductIdOverride"].get<int>();
	}
	if(headsetData["dscVersion"].is_number()){
		headsetConfig.dscVersion = headsetData["dscVersion"].get<int>();
	}
	if(headsetData["dscSliceCount"].is_number()){
		headsetConfig.dscSliceCount = headsetData["dscSliceCount"].get<int>();
	}
	if(headsetData["dscBPPx16"].is_number()){
		headsetConfig.dscBPPx16 = headsetData["dscBPPx16"].get<int>();
	}
	if(headsetData["forceEnable"].is_boolean()){
		headsetConfig.forceEnable = headsetData["forceEnable"].get<bool>();
	}
	if(headsetData["parallelProjection"].is_boolean()){
		headsetConfig.parallelProjection = headsetData["parallelProjection"].get<bool>();
	}
	
	if(json& hiddenAreaJson = headsetData["hiddenArea"]; hiddenAreaJson.is_object()){
		auto& newHiddenArea = headsetConfig.hiddenArea;
		if(hiddenAreaJson["enable"].is_boolean()){ newHiddenArea.enable = hiddenAreaJson["enable"].get<bool>(); }
		if(hiddenAreaJson["testMode"].is_boolean()){ newHiddenArea.testMode = hiddenAreaJson["testMode"].get<bool>(); }
		if(hiddenAreaJson["detailLevel"].is_number()){ newHiddenArea.detailLevel = hiddenAreaJson["detailLevel"].get<int>(); }
		if(hiddenAreaJson["radiusTopOuter"].is_number()){ newHiddenArea.radiusTopOuter = hiddenAreaJson["radiusTopOuter"].get<double>(); }
		if(hiddenAreaJson["radiusTopInner"].is_number()){ newHiddenArea.radiusTopInner = hiddenAreaJson["radiusTopInner"].get<double>(); }
		if(hiddenAreaJson["radiusBottomInner"].is_number()){ newHiddenArea.radiusBottomInner = hiddenAreaJson["radiusBottomInner"].get<double>(); }
		if(hiddenAreaJson["radiusBottomOuter"].is_number()){ newHiddenArea.radiusBottomOuter = hiddenAreaJson["radiusBottomOuter"].get<double>(); }
	}
	if(json& stationaryDimmingJson = headsetData["stationaryDimming"]; stationaryDimmingJson.is_object()){
		auto& newStationaryDimming = headsetConfig.stationaryDimming;
		if(stationaryDimmingJson["enable"].is_boolean()){ newStationaryDimming.enable = stationaryDimmingJson["enable"].get<bool>(); }
		if(stationaryDimmingJson["movementThreshold"].is_number()){ newStationaryDimming.movementThreshold = stationaryDimmingJson["movementThreshold"].get<double>(); }
		if(stationaryDimmingJson["movementTime"].is_number()){ newStationaryDimming.movementTime = stationaryDimmingJson["movementTime"].get<double>(); }
		if(stationaryDimmingJson["dimBrightnessPercent"].is_number()){ newStationaryDimming.dimBrightnessPercent = stationaryDimmingJson["dimBrightnessPercent"].get<double>(); }
		if(stationaryDimmingJson["dimSeconds"].is_number()){ newStationaryDimming.dimSeconds = stationaryDimmingJson["dimSeconds"].get<double>(); }
		if(stationaryDimmingJson["brightenSeconds"].is_number()){ newStationaryDimming.brightenSeconds = stationaryDimmingJson["brightenSeconds"].get<double>(); }
	}
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
			json headsetData = data["meganeX8K"];
			parseBaseHeadsetConfig(headsetData, newConfig.meganeX8K);
		}
		if(data["dreamAir"].is_object()){
			json headsetData = data["dreamAir"];
			parseBaseHeadsetConfig(headsetData, newConfig.dreamAir);
		}
		if(data["generalHeadset"].is_object()){
			json generalHeadsetData = data["generalHeadset"];
			if(generalHeadsetData["useViveBluetooth"].is_boolean()){
				newConfig.generalHeadset.useViveBluetooth = generalHeadsetData["useViveBluetooth"].get<bool>();
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
			if(customShaderData["saturation"].is_number()){
				newConfig.customShader.saturation = customShaderData["saturation"].get<double>();
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
			if(customShaderData["samplingFilter"].is_string()){
				newConfig.customShader.samplingFilter = customShaderData["samplingFilter"].get<std::string>();
			}
			if(customShaderData["samplingFilterFXAA2SharpenStrength"].is_number()){
				newConfig.customShader.samplingFilterFXAA2SharpenStrength = customShaderData["samplingFilterFXAA2SharpenStrength"].get<double>();
			}
			if(customShaderData["samplingFilterFXAA2SharpenClamp"].is_number()){
				newConfig.customShader.samplingFilterFXAA2SharpenClamp = customShaderData["samplingFilterFXAA2SharpenClamp"].get<double>();
			}
			if(customShaderData["samplingFilterFXAA2CASStrength"].is_number()){
				newConfig.customShader.samplingFilterFXAA2CASStrength = customShaderData["samplingFilterFXAA2CASStrength"].get<double>();
			}
			if(customShaderData["samplingFilterFXAA2CASContrast"].is_number()){
				newConfig.customShader.samplingFilterFXAA2CASContrast = customShaderData["samplingFilterFXAA2CASContrast"].get<double>();
			}
			if(customShaderData["samplingFilterLumaSharpenStrength"].is_number()){
				newConfig.customShader.samplingFilterLumaSharpenStrength = customShaderData["samplingFilterLumaSharpenStrength"].get<double>();
			}
			if(customShaderData["samplingFilterLumaSharpenClamp"].is_number()){
				newConfig.customShader.samplingFilterLumaSharpenClamp = customShaderData["samplingFilterLumaSharpenClamp"].get<double>();
			}
			if(customShaderData["samplingFilterLumaSharpenPattern"].is_number()){
				newConfig.customShader.samplingFilterLumaSharpenPattern = customShaderData["samplingFilterLumaSharpenPattern"].get<int>();
			}
			if(customShaderData["samplingFilterLumaSharpenRadius"].is_number()){
				newConfig.customShader.samplingFilterLumaSharpenRadius = customShaderData["samplingFilterLumaSharpenRadius"].get<double>();
			}
 			if(customShaderData["samplingFilterCASStrength"].is_number()){
 				newConfig.customShader.samplingFilterCASStrength = customShaderData["samplingFilterCASStrength"].get<double>();
 			}
			if(customShaderData["samplingFilterCASContrast"].is_number()){
				newConfig.customShader.samplingFilterCASContrast = customShaderData["samplingFilterCASContrast"].get<double>();
			}
			if(customShaderData["colorMultiplier"].is_object()){
				ConfigColor &colorMultiplier = newConfig.customShader.colorMultiplier;
				if(customShaderData["colorMultiplier"]["r"].is_number()){ colorMultiplier.r = customShaderData["colorMultiplier"]["r"].get<double>(); }
				if(customShaderData["colorMultiplier"]["g"].is_number()){ colorMultiplier.g = customShaderData["colorMultiplier"]["g"].get<double>(); }
				if(customShaderData["colorMultiplier"]["b"].is_number()){ colorMultiplier.b = customShaderData["colorMultiplier"]["b"].get<double>(); }
			}
		}
		if(data["forceTracking"].is_boolean()){
			newConfig.forceTracking = data["forceTracking"].get<bool>();
		}
		if(data["takeCompositorScreenshots"].is_boolean()){
			newConfig.takeCompositorScreenshots = data["takeCompositorScreenshots"].get<bool>();
		}
		if(data["onlyHandlePrivateFunctionality"].is_boolean()){
			newConfig.onlyHandlePrivateFunctionality = data["onlyHandlePrivateFunctionality"].get<bool>();
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
		if(data["offsetX"].is_number()){
			profile.offsetX = data["offsetX"].get<double>();
		}
		if(data["offsetY"].is_number()){
			profile.offsetY = data["offsetY"].get<double>();
		}
		if(data["legacySmoothing"].is_boolean()){
			profile.legacySmoothing = data["legacySmoothing"].get<bool>();
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

ordered_json baseHeadsetInfo(const Config::BaseHeadsetConfig& headsetConfig){
	return {
		{"enable", headsetConfig.enable},
		{"ipd", headsetConfig.ipd},
		{"ipdOffset", headsetConfig.ipdOffset},
		{"blackLevel", headsetConfig.blackLevel},
		{"colorMultiplier", {
			{"r", headsetConfig.colorMultiplier.r},
			{"g", headsetConfig.colorMultiplier.g},
			{"b", headsetConfig.colorMultiplier.b},
		}},
		{"distortionProfile", headsetConfig.distortionProfile},
		{"distortionZoom", headsetConfig.distortionZoom},
		{"fovZoom", headsetConfig.fovZoom},
		{"subpixelShift", headsetConfig.subpixelShift},
		{"resolutionX", headsetConfig.resolutionX},
		{"resolutionY", headsetConfig.resolutionY},
		{"displayRotation", headsetConfig.displayRotation},
		{"maxFovX", headsetConfig.maxFovX},
		{"maxFovY", headsetConfig.maxFovY},
		{"distortionMeshResolution", headsetConfig.distortionMeshResolution},
		{"fovBurnInPrevention", headsetConfig.fovBurnInPrevention},
		{"renderResolutionMultiplierX", headsetConfig.renderResolutionMultiplierX},
		{"renderResolutionMultiplierY", headsetConfig.renderResolutionMultiplierY},
		{"superSamplingFilterPercent", headsetConfig.superSamplingFilterPercent},
		{"secondsFromVsyncToPhotons", headsetConfig.secondsFromVsyncToPhotons},
		{"secondsFromPhotonsToVblank", headsetConfig.secondsFromPhotonsToVblank},
		{"eyeRotation", headsetConfig.eyeRotation},
		{"disableEye", headsetConfig.disableEye},
		{"disableEyeDecreaseFov", headsetConfig.disableEyeDecreaseFov},
		{"useViveBluetooth", headsetConfig.useViveBluetooth},
		{"directMode", headsetConfig.directMode},
		{"replaceIcons", headsetConfig.replaceIcons},
		{"edidVendorIdOverride", headsetConfig.edidVendorIdOverride},
		{"edidProductIdOverride", headsetConfig.edidProductIdOverride},
		{"dscVersion", headsetConfig.dscVersion},
		{"dscSliceCount", headsetConfig.dscSliceCount},
		{"dscBPPx16", headsetConfig.dscBPPx16},
		{"forceEnable", headsetConfig.forceEnable},
		{"parallelProjection", headsetConfig.parallelProjection},
		{"hiddenArea", {
			{"enable", headsetConfig.hiddenArea.enable},
			{"testMode", headsetConfig.hiddenArea.testMode},
			{"detailLevel", headsetConfig.hiddenArea.detailLevel},
			{"radiusTopOuter", headsetConfig.hiddenArea.radiusTopOuter},
			{"radiusTopInner", headsetConfig.hiddenArea.radiusTopInner},
			{"radiusBottomInner", headsetConfig.hiddenArea.radiusBottomInner},
			{"radiusBottomOuter", headsetConfig.hiddenArea.radiusBottomOuter},
		}},
		{"stationaryDimming", {
			{"enable", headsetConfig.stationaryDimming.enable},
			{"movementThreshold", headsetConfig.stationaryDimming.movementThreshold},
			{"movementTime", headsetConfig.stationaryDimming.movementTime},
			{"dimBrightnessPercent", headsetConfig.stationaryDimming.dimBrightnessPercent},
			{"dimSeconds", headsetConfig.stationaryDimming.dimSeconds},
			{"brightenSeconds", headsetConfig.stationaryDimming.brightenSeconds},
		}},
	};
}
void ConfigLoader::WriteInfo(){
	std::string infoPath = GetConfigFolder() + "info.json";
	std::ofstream infoFile(infoPath);
	if(!infoFile.is_open()){
		#ifdef _WIN32
		DriverLog("Failed to open info.json for writing: %d", GetLastError());
		#else
		DriverLog("Failed to open info.json for writing");
		#endif
		return;
	}
	Config defaultSettings = {};
	std::map<std::string,int> emptyObject = {};
	ordered_json data = {
		{"about", "This file is not for configuration. It provides info from the driver for other utilities to use."},
		{"defaultSettings", {
			{"meganeX8K", baseHeadsetInfo(defaultSettings.meganeX8K)},
			{"dreamAir", baseHeadsetInfo(defaultSettings.dreamAir)},
			{"generalHeadset", {
				{"useViveBluetooth", defaultSettings.generalHeadset.useViveBluetooth},
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
				{"saturation", defaultSettings.customShader.saturation},
				{"gamma", defaultSettings.customShader.gamma},
				{"subpixelShift", defaultSettings.customShader.subpixelShift},
				{"disableMuraCorrection", defaultSettings.customShader.disableMuraCorrection},
				{"disableBlackLevels", defaultSettings.customShader.disableBlackLevels},
				{"srgbColorCorrection", defaultSettings.customShader.srgbColorCorrection},
				{"srgbColorCorrectionMatrix", defaultSettings.customShader.srgbColorCorrectionMatrix},
				{"lensColorCorrection", defaultSettings.customShader.lensColorCorrection},
				{"dither10Bit", defaultSettings.customShader.dither10Bit},
				{"samplingFilter", defaultSettings.customShader.samplingFilter},
				{"samplingFilterFXAA2SharpenStrength", defaultSettings.customShader.samplingFilterFXAA2SharpenStrength},
				{"samplingFilterFXAA2SharpenClamp", defaultSettings.customShader.samplingFilterFXAA2SharpenClamp},
				{"samplingFilterFXAA2CASStrength", defaultSettings.customShader.samplingFilterFXAA2CASStrength},
				{"samplingFilterFXAA2CASContrast", defaultSettings.customShader.samplingFilterFXAA2CASContrast},
				{"samplingFilterLumaSharpenStrength", defaultSettings.customShader.samplingFilterLumaSharpenStrength},
				{"samplingFilterLumaSharpenClamp", defaultSettings.customShader.samplingFilterLumaSharpenClamp},
				{"samplingFilterLumaSharpenPattern", defaultSettings.customShader.samplingFilterLumaSharpenPattern},
				{"samplingFilterLumaSharpenRadius", defaultSettings.customShader.samplingFilterLumaSharpenRadius},
				{"samplingFilterCASStrength", defaultSettings.customShader.samplingFilterCASStrength},
				{"samplingFilterCASContrast", defaultSettings.customShader.samplingFilterCASContrast},
				{"colorMultiplier", {
					{"r", defaultSettings.customShader.colorMultiplier.r},
					{"g", defaultSettings.customShader.colorMultiplier.g},
					{"b", defaultSettings.customShader.colorMultiplier.b},
				}},
			}},
			{"forceTracking", defaultSettings.forceTracking},
			{"takeCompositorScreenshots", defaultSettings.takeCompositorScreenshots},
			{"onlyHandlePrivateFunctionality", defaultSettings.onlyHandlePrivateFunctionality},
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
			{"outputResolutionX", info.outputResolutionX},
			{"outputResolutionY", info.outputResolutionY},
		}},
		{"connectedHeadset", (int)info.connectedHeadset},
		{"nonNativeHeadsetFound", info.nonNativeHeadsetFound},
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
		#ifdef _WIN32
		DriverLog("Failed to open info.json for reading: %d", GetLastError());
		#else
		DriverLog("Failed to open info.json for reading");
		#endif
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
			info.connectedHeadset = (Config::HeadsetType)data["connectedHeadset"].get<int>();
		}
		if(data["resolution"].is_object()){
			auto res = data["resolution"];
			if(res["fovX"].is_number()) info.renderFovX = res["fovX"].get<double>();
			if(res["fovY"].is_number()) info.renderFovY = res["fovY"].get<double>();
			if(res["fovMaxX"].is_number()) info.renderFovMaxX = res["fovMaxX"].get<double>();
			if(res["fovMaxY"].is_number()) info.renderFovMaxY = res["fovMaxY"].get<double>();
			if(res["renderResolution1To1X"].is_number()) info.renderResolution1To1X = res["renderResolution1To1X"].get<int>();
			if(res["renderResolution1To1Y"].is_number()) info.renderResolution1To1Y = res["renderResolution1To1Y"].get<int>();
			if(res["renderResolution1To1Percent"].is_number()) info.renderResolution1To1Percent = res["renderResolution1To1Percent"].get<double>();
			if(res["renderResolution100PercentX"].is_number()) info.renderResolution100PercentX = res["renderResolution100PercentX"].get<int>();
			if(res["renderResolution100PercentY"].is_number()) info.renderResolution100PercentY = res["renderResolution100PercentY"].get<int>();
			if(res["outputResolutionX"].is_number()) info.outputResolutionX = res["outputResolutionX"].get<int>();
			if(res["outputResolutionY"].is_number()) info.outputResolutionY = res["outputResolutionY"].get<int>();
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
#ifdef _WIN32
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
#elif __linux__
// use the c inotify system to watch for changes to the config file
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void ConfigLoader::WatcherThread(){
	std::string configPath = GetConfigFolder();
	int fd = inotify_init();
	if(fd == -1){
		DriverLog("Error initializing inotify\n");
		return;
	}
	int wd = inotify_add_watch(fd, configPath.c_str(), IN_MODIFY | IN_CREATE);
	if(wd == -1){
		DriverLog("Error adding inotify watch to config folder\n");
		return;
	}
	char buffer[sizeof(struct inotify_event) * 16];
	while(started){
		int length = read(fd, buffer, sizeof(buffer));
		if(length == -1){
			DriverLog("Error reading inotify events\n");
			break;
		}
		// process events
		int i = 0;
		
		bool hasReloadedConfig = false;
		bool hasReloadedInfo = false;
		while(i < length){
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			if(event->mask & IN_MODIFY || event->mask & IN_CREATE){
				if(event->len){
					std::string fileName = event->name;
					if(fileName == "settings.json" && !hasReloadedConfig){
						DriverLog("Config file changed, reloading...");
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						while(driverConfig.hasBeenUpdated){
							// wait for the last config update to be used before doing another one
							std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
						}
						ParseConfig();
						hasReloadedConfig = true;
					}
					if(fileName == "info.json" && !hasReloadedInfo){
						DriverLog("Info file changed, reloading...");
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						ReadInfo();
						hasReloadedInfo = true;
					}
				}
			}
			i += sizeof(struct inotify_event) + event->len;
		}
	}
	inotify_rm_watch(fd, wd);
	close(fd);
}


void ConfigLoader::WatcherThreadDistortions(){
	std::string configPath = GetConfigFolder() + "Distortion/";
	int fd = inotify_init();
	if(fd == -1){
		DriverLog("Error initializing inotify\n");
		return;
	}
	int wd = inotify_add_watch(fd, configPath.c_str(), IN_MODIFY | IN_CREATE);
	if(wd == -1){
		DriverLog("Error adding inotify watch to distortion folder\n");
		return;
	}
	char buffer[sizeof(struct inotify_event) * 16];
	while(started){
		int length = read(fd, buffer, sizeof(buffer));
		if(length == -1){
			DriverLog("Error reading inotify events\n");
			break;
		}
		// process events
		int i = 0;
		while(i < length){
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			if(event->mask & IN_MODIFY || event->mask & IN_CREATE){
				if(event->len){
					std::string fileName = event->name;
					if(fileName.find(".json") != std::string::npos){
						DriverLog("Distortion profile changed, reloading...");
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							while(driverConfig.hasBeenUpdated){
								// wait for the last config update to be used before doing another one
								std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
							}
						ParseConfig();
						break;
					}
				}
			}
			i += sizeof(struct inotify_event) + event->len;
		}
	}
	inotify_rm_watch(fd, wd);
	close(fd);
}
#endif

				
// only define settings that most users will change and are unlikely to have their default changed
// settings not defined here will easily be able to have their defaults changed in the future for everyone
std::string defaultConfig = R"({
	"meganeX8K": {
		"enable": true
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
	}catch(const std::exception& e){
		DriverLog("Failed to create settings.json %s", e.what());
	}
	
	// load config for the first time
	ParseConfig();
	
	try{
		if(watchInfo || driverConfig.onlyHandlePrivateFunctionality){
			ReadInfo();
		}else{
			WriteInfo();
			// start info thread
			// std::thread infoThread(&ConfigLoader::WriteInfoThread, this);
			// infoThread.detach();
		}
	}catch(const std::exception& e){
		DriverLog("Failed to manage info.json: %s", e.what());
	}
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