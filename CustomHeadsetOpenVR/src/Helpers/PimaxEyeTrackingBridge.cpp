#include "PimaxEyeTrackingBridge.h"
#include "../Driver/DriverLog.h"

#include <chrono>
#include <cmath>

#ifdef PVR_EXISTS
	#include "C:/Program Files/Pimax/Sdk/Include/PVR_API.h"
#endif

PimaxEyeTrackingBridge::PimaxEyeTrackingBridge(){
}

PimaxEyeTrackingBridge::~PimaxEyeTrackingBridge(){
	Shutdown();
}

bool PimaxEyeTrackingBridge::Initialize(){
	if(initialized){
		return true;
	}
	
	initializeCount++;
	
#ifdef PVR_EXISTS
	// Initialize the eye tracking output component
	eyeTrackingOutput.ipd = ipd;
	eyeTrackingOutput.Initialize();
	
	if(!enabled){
		return false;
	}
	
	// Initialize PVR environment
	pvrResult result = pvr_initialise(&envHandle);
	if(result != pvr_success){
		if(initializeCount < 10){
			DriverLog("PimaxEyeTrackingBridge: Failed to initialize PVR environment, result: %d", result);
		}
		return false;
	}
	
	// Create a session to PVR runtime
	result = pvr_createSession(envHandle, &sessionHandle);
	if(result != pvr_success){
		if(initializeCount < 10){
			DriverLog("PimaxEyeTrackingBridge: Failed to create PVR session, result: %d", result);
		}
		pvr_shutdown(envHandle);
		envHandle = nullptr;
		return false;
	}
	
	// Set IPD in PVR runtime (ipd is in meters)
	pvr_setFloatConfig(sessionHandle, CONFIG_KEY_IPD, ipd);
	
	// Get HmdStatus to check if service is ready
	pvrHmdStatus hmdStatus = {};
	result = pvr_getHmdStatus(sessionHandle, &hmdStatus);
	if(result != pvr_success || !hmdStatus.ServiceReady || !hmdStatus.HmdPresent || hmdStatus.ShouldQuit){
		if(initializeCount < 10){
			DriverLog("PimaxEyeTrackingBridge: PVR service not ready, result: %d, serviceReady: %d, hmdPresent: %d, hmdMounted: %d, shouldQuit: %d\n", result, hmdStatus.ServiceReady, hmdStatus.HmdPresent, hmdStatus.HmdMounted, hmdStatus.ShouldQuit);
		}
		pvr_destroySession(sessionHandle);
		sessionHandle = nullptr;
		pvr_shutdown(envHandle);
		envHandle = nullptr;
		return false;
	}
	
	// Get SDK version for debugging
	const char* version = pvr_getVersionString(envHandle);
	DriverLog("PimaxEyeTrackingBridge: PVR API initialized successfully, version: %s", version);
	
	
	initialized = true;
	threadRunning = true;

	// Start the dedicated eye tracking thread running at 120Hz
	eyeTrackingThread = std::thread(&PimaxEyeTrackingBridge::EyeTrackingThread, this);
	
	DriverLog("PimaxEyeTrackingBridge: Initialization complete");
#else
	DriverLog("PimaxEyeTrackingBridge: Not built with the PVR API, eye tracking disabled");
#endif
	return initialized;
}

void PimaxEyeTrackingBridge::Shutdown(){
	threadRunning = false;
	if(eyeTrackingThread.joinable()){
		eyeTrackingThread.join();
	}

#ifdef PVR_EXISTS
	if(sessionHandle){
		pvr_destroySession(sessionHandle);
		sessionHandle = nullptr;
	}
	if(envHandle){
		pvr_shutdown(envHandle);
		envHandle = nullptr;
	}
#endif
	initialized = false;
	DriverLog("PimaxEyeTrackingBridge: Shutdown complete");
}

void PimaxEyeTrackingBridge::RunFrame(){
#ifdef PVR_EXISTS
	double now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000000.0;
	if(enabled){
		// If not initialized, attempt to re-initialize every 10 seconds
		if(!initialized || !threadRunning){
			double secondsSinceLastRestart = now - lastRestartTime;
			if(initialized){
				Shutdown();
				DriverLog("PimaxEyeTrackingBridge: PVR API shutdown due to error");
			}
			if(secondsSinceLastRestart >= 10.0){
				lastRestartTime = now;
				DriverLog("PimaxEyeTrackingBridge: Attempting to re-initialize PVR API");
				Initialize();
			}
		}
	}else{
		// If disabled, shutdown the PVR API
		if(initialized){
			Shutdown();
			DriverLog("PimaxEyeTrackingBridge: PVR API shutdown due to disabled state");
		}
	}
#endif
}

void PimaxEyeTrackingBridge::EyeTrackingThread(){

#ifdef PVR_EXISTS
	auto now = std::chrono::steady_clock::now();
	static std::chrono::steady_clock::time_point lastLogTime = now;
	while(threadRunning){
		auto startTime = std::chrono::high_resolution_clock::now();
		now = startTime;

		// Get the predicted display time for the current frame
		double displayTime = pvr_getTimeSeconds(envHandle);
		
		// Get eye tracking info from PVR
		pvrEyeTrackingInfo eyeTrackingInfo = {};
		pvrResult result = pvr_getEyeTrackingInfo(sessionHandle, displayTime, &eyeTrackingInfo);

		if(result == pvr_success){
			// Convert PVR eye tracking data to angles for EyeTrackingOutput
			// PVR provides gaze as tangent values, convert to angles in radians
			float leftAngleX = atan(eyeTrackingInfo.GazeTan[pvrEye_Left].x);
			float leftAngleY = atan(eyeTrackingInfo.GazeTan[pvrEye_Left].y);
			float rightAngleX = atan(eyeTrackingInfo.GazeTan[pvrEye_Right].x);
			float rightAngleY = atan(eyeTrackingInfo.GazeTan[pvrEye_Right].y);

			// // Apply horizontal eye rotation offset
			// float eyeRotationRad = eyeRotation * 3.14159265358979f / 180.0f;
			// leftAngleX -= eyeRotationRad;
			// rightAngleX += eyeRotationRad;

			// Set the eye tracking data
			eyeTrackingOutput.SetEyeTrackingData(leftAngleX, leftAngleY, rightAngleX, rightAngleY);
		}else{
			DriverLog("PimaxEyeTrackingBridge: Failed to get eye tracking info, result: %d", result);
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
		
		pvrHmdStatus hmdStatus = {};
		result = pvr_getHmdStatus(sessionHandle, &hmdStatus);
		if(result != pvr_success || hmdStatus.ShouldQuit){
			if(initializeCount < 10){
				DriverLog("PimaxEyeTrackingBridge: HMD session ending, result: %d, serviceReady: %d, hmdPresent: %d, hmdMounted: %d, shouldQuit: %d\n", result, hmdStatus.ServiceReady, hmdStatus.HmdPresent, hmdStatus.HmdMounted, hmdStatus.ShouldQuit);
			}
			threadRunning = false;
			break;
		}

		auto endTime = std::chrono::high_resolution_clock::now();
		double elapsedMs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count() / 1000000.0;

		// Log performance every 10 seconds
		double secondsSinceLastLog = std::chrono::duration<double>(now - lastLogTime).count();
		if(secondsSinceLastLog >= 10.0){
			lastLogTime = now;
			// DriverLog("PimaxEyeTrackingBridge: EyeTrackingThread took %.6f ms to gather eye tracking data", elapsedMs);
			// DriverLog("PimaxEyeTrackingBridge: PVR eye tracking data: left gaze (%.3f, %.3f), right gaze (%.3f, %.3f)", 
			// 	eyeTrackingInfo.GazeTan[pvrEye_Left].x, eyeTrackingInfo.GazeTan[pvrEye_Left].y, 
			// 	eyeTrackingInfo.GazeTan[pvrEye_Right].x, eyeTrackingInfo.GazeTan[pvrEye_Right].y);
			// DriverLog("PimaxEyeTrackingBridge: Focal Point: X=%.4f, Y=%.4f, Z=%.4f m\n", eyeTrackingOutput.eyeData.focalPointX, eyeTrackingOutput.eyeData.focalPointY, eyeTrackingOutput.eyeData.focalPointZ);
		}

		// Sleep to maintain 120Hz polling rate (8.333ms per frame), accounting for function runtime
		double targetIntervalUs = 8333.0;
		double sleepTimeUs = targetIntervalUs - elapsedMs * 1000.0;
		if(sleepTimeUs > 0.0){
			std::this_thread::sleep_for(std::chrono::microseconds((int)sleepTimeUs));
		}
	}
#endif
}

// Global instance
PimaxEyeTrackingBridge gPimaxEyeTrackingBridge;
