#pragma once
#include "Config.h"
#include <string>
#include <vector>
#include <mutex>


// This class loads config files and watches for changes to them, updating the global config object as needed.
class ConfigLoader{
public:
	// class to contain info from elsewhere in the driver to write to info.json
	// this is not structured in the same way as the json file so check WriteInfo in ConfigLoader.cpp
	class Info{
	public:
		double renderFovX = 0;
		double renderFovY = 0;
		double renderFovMaxX = 0;
		double renderFovMaxY = 0;
		double combinedFovX = 0;
		double combinedFovY = 0;
		uint32_t renderResolution1To1X = 0;
		uint32_t renderResolution1To1Y = 0;
		double renderResolution1To1Percent = 0;
		uint32_t renderResolution100PercentX = 0;
		uint32_t renderResolution100PercentY = 0;
		// output resolution of the compositor per eye
		uint32_t outputResolutionX = 0;
		uint32_t outputResolutionY = 0;
		std::string debugLog = "";
		std::string driverResources = "";
		std::string steamvrResources = "";
		Config::HeadsetType connectedHeadset = Config::HeadsetType::None;
		// if a headset that does not use the steamvr compositor is connected
		bool nonNativeHeadsetFound = false;
		// if the SteamVR dashboard is currently open
		bool isDashboardOpen = false;
		// used when watching info
		bool hasBeenUpdated = true;
		// notifies thread that it should be written (uses diagnostic thread)
		bool needToWrite = true;
	};
	Info info;

	// class to contain diagnostic data written to diagnostic.json at 4 times per second
	class DiagnosticInfo{
	public:
		// SteamVR server process ID
		uint32_t vrserverPID = 0;
		
		// Eye tracking data
		bool eyeTrackingValid = false;
		float leftAngleX = 0;
		float leftAngleY = 0;
		float rightAngleX = 0;
		float rightAngleY = 0;
		float focalPointX = 0;
		float focalPointY = 0;
		float focalPointZ = 0;
	};
	DiagnosticInfo diagnosticInfo;

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
	// write the diagnostic.json file to disk (high frequency, 4 times per second)
	void WriteDiagnosticInfo();
	// start the config parser
	void Start();
	// thread to write info
	// void WriteInfoThread();
	// thread to write diagnostic info at 4 times per second
	void WriteDiagnosticInfoThread();
	// thread to watch for file changes
	void WatcherThread();
	// thread for watching distortions if enabled
	void WatcherThreadDistortions();
	// get folder with trailin slash for config files
	std::string GetConfigFolder();
private:
	bool hasLoggedConfigFileNotFound = false;
	std::mutex infoWriteLock;
	std::mutex diagnosticWriteLock;
};

// global config loader object, used to load config files and watch for changes
extern ConfigLoader driverConfigLoader;

// check if custom shaders are enabled for the current headset type
inline bool IsCustomShaderEnabled(){
	if(!driverConfig.customShader.enable){
		return false;
	}
	if(driverConfigLoader.info.connectedHeadset == Config::HeadsetType::MeganeX8K){
		return driverConfig.customShader.enableForMeganeX8K;
	}
	if(driverConfigLoader.info.connectedHeadset == Config::HeadsetType::DreamAir){
		return driverConfig.customShader.enableForDreamAir;
	}
	// all other headsets that do not have explicit toggles
	if(driverConfigLoader.info.connectedHeadset != Config::HeadsetType::None){
		return driverConfig.customShader.enableForOther;
	}
	return false;
}