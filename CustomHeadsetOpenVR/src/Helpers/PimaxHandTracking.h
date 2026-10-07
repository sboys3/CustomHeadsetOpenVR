#pragma once

// A class that acquires hand tracking data from the PVR API and forwards it to SteamVR
class PimaxHandTracking {
public:
	PimaxHandTracking();
	~PimaxHandTracking();
	
	// Stop hand tracking and disconnect the tracked devices
	void Stop();
	
	// Manage the hand tracking devices
	// Called from the PVR background thread
	void RunBackground();
	
	// Update the hand tracking data
	// Called from the PVR tracking thread
	void RunFrame();
	
private:
	bool probed = false;
	bool supported = false;
	bool pvrHandApiAvailable = false;
};

extern PimaxHandTracking pimaxHandTracking;