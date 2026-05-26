#pragma once

#include "openvr_driver.h"
#include "EyeTrackingOutput.h"

#include <thread>
#include <atomic>

// Check if PVR API is available on the system
// You will need to install the Pimax SDK in order for this to work.
// Get the windows SDK from here: https://developer.pimax.com/sdk/xr/pc-xr-sdk.html
// If the header exists, define PVR_EXISTS to enable eye tracking bridge functionality
#if __has_include("C:/Program Files/Pimax/Sdk/Include/PVR_API.h")
	#define PVR_EXISTS
	// Forward-declared pointers to avoid including the full PVR API header
	struct _pvrEnv;
	struct _pvrSession;
#endif

class PimaxEyeTrackingBridge {
public:
	PimaxEyeTrackingBridge();
	~PimaxEyeTrackingBridge();

	// Initialize the PVR API and prepare for eye tracking data acquisition
	// Returns true if initialization was successful
	bool Initialize();

	// Shutdown the PVR API and clean up resources
	void Shutdown();

	// Called every frame to attempt re-initialization if not connected and forward eye tracking data to SteamVR
	void RunFrame();

	float ipd = 63.0f;
	float eyeRotation = 0;
	bool enabled = true;
private:
	// Dedicated thread function that polls PVR at 120Hz and pushes data to EyeTrackingOutput
	void EyeTrackingThread();

	std::atomic<bool> initialized = false;
	std::atomic<bool> threadRunning = false;
	std::thread eyeTrackingThread;
	double lastRestartTime = 0.0;
	int initializeCount = 0;

#ifdef PVR_EXISTS
	_pvrEnv* envHandle = nullptr;
	_pvrSession* sessionHandle = nullptr;
#endif
};

// Global instance of the eye tracking bridge
// This allows access from anywhere in the codebase
extern PimaxEyeTrackingBridge gPimaxEyeTrackingBridge;
