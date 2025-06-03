#pragma once
#include "Config.h"
#include <string>
#include <vector>


// This class loads config files and watches for changes to them, updating the global config object as needed.
class ConfigLoader{
public:
	// class to contain info from elsewhere in the driver to write to info.json
	// this is not structured in the same way as the json file so check WriteInfo in ConfigLoader.cpp
	class Info{
	public:
		uint32_t renderResolution1To1X = 0;
		uint32_t renderResolution1To1Y = 0;
		double renderResolution1To1Percent = 0;
		uint32_t renderResolution100PercentX = 0;
		uint32_t renderResolution100PercentY = 0;
		std::string debugLog = "";
		std::string driverResources = "";
		std::string steamvrResources = "";
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