#pragma once
#include "Config.h"
#include <string>
#include <vector>


// This class loads config files and watches for changes to them, updating the global config object as needed.
class ConfigLoader{
public:
	bool started = false;
	// parse the config file into the global config object
	void ParseConfig();
	// load a distortion profile config from disk
	DistortionProfileConfig ParseDistortionConfig(std::string name);
	// start the config parser
	void Start();
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