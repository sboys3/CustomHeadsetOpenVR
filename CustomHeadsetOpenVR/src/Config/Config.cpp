#include "Config.h"
Config driverConfig = {};
Config driverConfigOld = {};
Config defaultDriverConfig = {};
std::mutex driverConfigLock;
std::string driverVersion = "1.2.0-beta.0";