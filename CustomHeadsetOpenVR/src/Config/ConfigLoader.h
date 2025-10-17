#pragma once
#include "Config.h"
#include <string>
#include <vector>


// This class loads config files and watches for changes to them, updating the global config object as needed.
class ConfigLoader{
public:
	enum HeadsetType{
		None = 0,
		Other = 1,
		MeganeX8K = 2,
		Vive = 3,
	};
	// class to contain info from elsewhere in the driver to write to info.json
	// this is not structured in the same way as the json file so check WriteInfo in ConfigLoader.cpp
	class Info{
	public:
		double renderFovX = 0;
		double renderFovY = 0;
		double renderFovMaxX = 0;
		double renderFovMaxY = 0;
		uint32_t renderResolution1To1X = 0;
		uint32_t renderResolution1To1Y = 0;
		double renderResolution1To1Percent = 0;
		uint32_t renderResolution100PercentX = 0;
		uint32_t renderResolution100PercentY = 0;
		std::string debugLog = "";
		std::string driverResources = "";
		std::string steamvrResources = "";
		HeadsetType connectedHeadset = HeadsetType::None;
		// used when watching info
		bool hasBeenUpdated = true;
	};
	Info info;

	bool started = false;
	// read from info. used in the compositor to get info from the main driver
	bool watchInfo = false;
	// parse the config file into the global config object
	void ParseConfig();
	// load a distortion profile config from disk
	DistortionProfileConfig ParseDistortionConfig(std::string name);
	// save the info.json file to disk
	void WriteInfo();
	// read the info.json file from disk
	void ReadInfo();
	// start the config parser
	void Start();
	// thread to write info
	// void WriteInfoThread();
	// thread to watch for file changes
	void WatcherThread();
	// thread for watching distortions if enabled
	void WatcherThreadDistortions();
	// get folder with trailin slash for config files
	std::string GetConfigFolder();
private:
	bool hasLoggedConfigFileNotFound = false;
};

// global config loader object, used to load config files and watch for changes
extern ConfigLoader driverConfigLoader;

// check if custom shaders are enabled for the current headset type
inline bool IsCustomShaderEnabled(){
	if(!driverConfig.customShader.enable){
		return false;
	}
	if(driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::MeganeX8K){
		return driverConfig.customShader.enableForMeganeX8K;
	}
	// all other headsets that do not have explicit toggles
	if(driverConfigLoader.info.connectedHeadset != ConfigLoader::HeadsetType::None){
		return driverConfig.customShader.enableForOther;
	}
	return false;
}