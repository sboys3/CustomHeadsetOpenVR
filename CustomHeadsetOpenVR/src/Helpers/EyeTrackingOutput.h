#pragma once

#include "openvr_driver.h"
#include <mutex>

// A class that manages eye tracking data and forwards it to SteamVR in a thread-safe manner
class EyeTrackingOutput {
public:
	class EyeTrackingData{
	public:
		// Angles in radians. 0 is looking straight ahead. Positive X is looking right. Positive Y is looking up. 
		float leftAngleX = 0;
		float leftAngleY = 0;
		float rightAngleX = 0;
		float rightAngleY = 0;
		
		// Focal point in meters. This is the point in 3D space that the user is looking at.
		float focalPointX = 0;
		float focalPointY = 0;
		float focalPointZ = 0;
		
		// Origin point in meters. Probably 0
		float originPointX = 0;
		float originPointY = 0;
		float originPointZ = 0;
		
		// Timestamp in unix epoch seconds. This is the time that the eye tracking data was captured.
		double timestamp = 0;
	};
	
	EyeTrackingOutput();
	~EyeTrackingOutput();
	
	// Initialize the eye tracking system for a device
	void Initialize();
	
	// Set the eye tracking data in a thread-safe manner using an internal lock
	// This needs to be fully filled out
	void SetEyeTrackingData(EyeTrackingData data);
	
	// Set the eye tracking data from angles only
	void SetEyeTrackingData(float leftAngleX, float leftAngleY, float rightAngleX, float rightAngleY, double timestamp=0);
	
	// Called every frame to pass the eye tracking data to SteamVR
	// Probably needs to be called from the main thread
	// Called in device provider
	void RunFrame();
	
	// IPD in mm
	// used to calculate the focal point.
	float ipd = 63.0f;

	std::mutex dataLock = {};
	EyeTrackingData eyeData = {};
	vr::VRInputComponentHandle_t eyeTrackingComponentHandle = vr::k_ulInvalidInputComponentHandle;
	bool initialized = false;
};

extern EyeTrackingOutput eyeTrackingOutput;