#include "Config.h"
Config driverConfig = {};
Config driverConfigOld = {};
std::mutex driverConfigLock;
std::string driverVersion = "1.0.0-beta.1";