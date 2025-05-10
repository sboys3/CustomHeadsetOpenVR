#pragma once
#include "Config.h"
#include <string>

class ConfigLoader{
public:
	bool started = false;
	// parse the config file into the global config object
	void ParseConfig();
	// start the config parser
	void Start();
	// thread to watch for file changes
	void WatcherThread();
	// get folder with trailin slash for config files
	std::string GetConfigFolder();
private:
	bool hasLoggedConfigFileNotFound = false;
};