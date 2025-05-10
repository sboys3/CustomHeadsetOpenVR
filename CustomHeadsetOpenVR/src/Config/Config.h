#pragma once
#include <mutex>


class Config{
public:
	class MeganeX8KConfig{
	public:
		// if the MeganeX superlight 8K should be shimmed
		bool enable = true;
		// ipd in mm
		double ipd = 63.0;
		// minimum black levels from 0 to 1
		double blackLevel = 0;
	};
	// config for the MeganeX superlight 8K
	MeganeX8KConfig meganeX8K;
	
	// if the config has been changes and should be reloaded
	// this will be set the false at the end of RunFrame
	bool hasBeenUpdated = true;
	
	// lock for the config to prevent updates
	std::mutex configLock;
};

// global config object
extern Config driverConfig; 