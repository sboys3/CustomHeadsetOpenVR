#include "Config.h"
Config driverConfig = {};
Config driverConfigOld = {};
std::mutex driverConfigLock;
std::string driverVersion = "0.5.1";