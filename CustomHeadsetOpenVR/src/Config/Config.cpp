#include "Config.h"
Config driverConfig = {};
Config driverConfigOld = {};
std::mutex driverConfigLock;
std::string driverVersion = "1.1.0";