#include "EyeTrackingOutput.h"
#include "../Driver/DriverLog.h"

#include <chrono>

EyeTrackingOutput::EyeTrackingOutput(){
}

EyeTrackingOutput::~EyeTrackingOutput(){
}

void EyeTrackingOutput::Initialize(){	
	// Create the eye tracking component in SteamVR
	vr::IVRDriverInput* driverInput = vr::VRDriverInput();
	if(driverInput){
		vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
		vr::VRProperties()->SetBoolProperty(container, vr::Prop_SupportsXrEyeGazeInteraction_Bool, true);
		vr::EVRInputError inputError = driverInput->CreateEyeTrackingComponent(container, "/eyetracking", &eyeTrackingComponentHandle);
		if(inputError == vr::VRInputError_None){
			DriverLog("EyeTrackingOutput: Created SteamVR eye tracking component");
			initialized = true;
		}else if(!eyeTrackingComponentHandle){
			DriverLog("EyeTrackingOutput: Failed to create SteamVR eye tracking component, error: %d", inputError);
		}
	} else if(!eyeTrackingComponentHandle) {
		DriverLog("EyeTrackingOutput: IVRDriverInput interface not available");
	}
}

void EyeTrackingOutput::SetEyeTrackingData(EyeTrackingData data){
	if(!data.timestamp){
		data.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000000.0;
	}
	std::lock_guard<std::mutex> lock(dataLock);
	eyeData = data;
}

void EyeTrackingOutput::SetEyeTrackingData(float leftAngleX, float leftAngleY, float rightAngleX, float rightAngleY, double timestamp){
	float tanLeftX = tan(leftAngleX);
	float tanLeftY = tan(leftAngleY);
	float tanRightX = tan(rightAngleX);
	float tanRightY = tan(rightAngleY);
	
	float intersectionDistance = ipd / 1000.0f / (tanLeftX - tanRightX);
	if(intersectionDistance < 0.001f || intersectionDistance > 20.0f || !isfinite(intersectionDistance)) {
		intersectionDistance = 20.0f;
	}

	float focalPointZ = intersectionDistance;
	float focalPointX = (tanLeftX + tanRightX) / 2.0 * focalPointZ;
	float focalPointY = (tanLeftY + tanRightY) / 2.0 * focalPointZ;
	
	EyeTrackingData data;
	data.leftAngleX = leftAngleX;
	data.leftAngleY = leftAngleY;
	data.rightAngleX = rightAngleX;
	data.rightAngleY = rightAngleY;
	data.focalPointX = focalPointX;
	data.focalPointY = focalPointY;
	data.focalPointZ = focalPointZ;
	data.timestamp = timestamp;
	SetEyeTrackingData(data);
}

void EyeTrackingOutput::RunFrame(){
	if(!initialized){
		return;
	}
	EyeTrackingData localData;
	{
		std::lock_guard<std::mutex> lock(dataLock);
		localData = eyeData;
	}

	vr::IVRDriverInput* driverInput = vr::VRDriverInput();
	if(driverInput && eyeTrackingComponentHandle != vr::k_ulInvalidInputComponentHandle && localData.timestamp > 0.0){
		vr::VREyeTrackingData_t eyeTrackingData;
		double now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000000.0;
		double timeOffset = localData.timestamp - now;
		double timeSinceUpdate = now - localData.timestamp;
		if(timeSinceUpdate < 1.0){
			eyeTrackingData.bActive = true;
			eyeTrackingData.bValid = true;
			eyeTrackingData.bTracked = true;
			eyeTrackingData.vGazeOrigin.v[0] = localData.originPointX;
			eyeTrackingData.vGazeOrigin.v[1] = localData.originPointY;
			eyeTrackingData.vGazeOrigin.v[2] = -localData.originPointZ;
			eyeTrackingData.vGazeTarget.v[0] = localData.focalPointX;
			eyeTrackingData.vGazeTarget.v[1] = localData.focalPointY;
			eyeTrackingData.vGazeTarget.v[2] = -localData.focalPointZ;
		} else {
			bool valid = timeSinceUpdate < 1.5;
			eyeTrackingData.bActive = valid;
			eyeTrackingData.bValid = valid;
			eyeTrackingData.bTracked = valid;
			eyeTrackingData.vGazeOrigin.v[0] = 0.0f;
			eyeTrackingData.vGazeOrigin.v[1] = 0.0f;
			eyeTrackingData.vGazeOrigin.v[2] = 0.0f;
			eyeTrackingData.vGazeTarget.v[0] = 0.0f;
			eyeTrackingData.vGazeTarget.v[1] = 0.0f;
			eyeTrackingData.vGazeTarget.v[2] = 1.0f;
			timeOffset = 0;
		}
			
		driverInput->UpdateEyeTrackingComponent(eyeTrackingComponentHandle, &eyeTrackingData, timeOffset);
	}
}


EyeTrackingOutput eyeTrackingOutput = {};