#include "DeviceShim.h"
#include "DriverLog.h"

ShimTrackedDeviceDriver::ShimTrackedDeviceDriver(ShimDefinition* shimDefinition, vr::ITrackedDeviceServerDriver* original){
	DriverLog("Creating ShimTrackedDeviceDriver");
	this->shimDefinition = shimDefinition;
	if(!shimDefinition->trackedDevice){
		shimDefinition->trackedDevice = original;
	}
}

ShimDisplayComponent::ShimDisplayComponent(ShimDefinition* shimDefinition, vr::IVRDisplayComponent* original){
	DriverLog("Creating ShimDisplayComponent");
	this->shimDefinition = shimDefinition;
	if(!shimDefinition->displayComponent){
		shimDefinition->displayComponent = original;
	}
}

// shim the component function to also apply shims to components
void *ShimTrackedDeviceDriver::GetComponent(const char *pchComponentNameAndVersion){
	void* returnValue = nullptr;
	if(shimDefinition->shimActive){
		if(!shimDefinition->PreTrackedDeviceGetComponent(pchComponentNameAndVersion, returnValue)){
			return returnValue;
		}
	}
	if(strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version) == 0 && shimDefinition->shimActive){
		returnValue = new ShimDisplayComponent(shimDefinition, (vr::IVRDisplayComponent*)shimDefinition->trackedDevice->GetComponent(pchComponentNameAndVersion));
	}else{
		returnValue = shimDefinition->trackedDevice->GetComponent(pchComponentNameAndVersion);
	}
	if(shimDefinition->shimActive){
		shimDefinition->PosTrackedDeviceGetComponent(pchComponentNameAndVersion, returnValue);
	}
	return returnValue;
}


// vr::EVRInitError ShimTrackedDeviceDriver::Activate( uint32_t unObjectId ){
// 	vr::EVRInitError returnValue;
// 	if(shimDefinition->shimActive){
// 		if(!shimDefinition->PreTrackedDeviceActivate(unObjectId, returnValue)){
// 			return returnValue;
// 		}
// 	}
// 	returnValue = shimDefinition->trackedDevice->Activate(unObjectId);
// 	if(shimDefinition->shimActive){
// 		shimDefinition->PosTrackedDeviceActivate(unObjectId, returnValue);
// 	}
// 	return returnValue;
// }





#define COMMA ,

// DriverLog("Shim call: " #shimClass "::" #functionName "(" #argumentList ")" "\n");
#define SHIM_CALL_RETURNS(shimClass, functionName, shimClassFunctionName, shimObject, parameters, argumentList, returnType) \
returnType shimClass::functionName(parameters){ \
	 \
	returnType returnValue; \
	if(shimDefinition->shimActive){ \
		if(!shimDefinition->Pre##shimClassFunctionName##functionName(argumentList, returnValue)){ \
			return returnValue; \
		} \
	} \
	returnValue = shimDefinition->shimObject->functionName(argumentList); \
	if(shimDefinition->shimActive){ \
		shimDefinition->Pos##shimClassFunctionName##functionName(argumentList, returnValue); \
	} \
	return returnValue; \
}

// DriverLog("Shim call: " #shimClass "::" #functionName "(" ")" "\n");
#define SHIM_CALL_RETURNS_NO_ARGS(shimClass, functionName, shimClassFunctionName, shimObject, returnType) \
returnType shimClass::functionName(){ \
	 \
	returnType returnValue; \
	if(shimDefinition->shimActive){ \
		if(!shimDefinition->Pre##shimClassFunctionName##functionName(returnValue)){ \
			return returnValue; \
		} \
	} \
	returnValue = shimDefinition->shimObject->functionName(); \
	if(shimDefinition->shimActive){ \
		shimDefinition->Pos##shimClassFunctionName##functionName(returnValue); \
	} \
	return returnValue; \
}

// DriverLog("Shim call: " #shimClass "::" #functionName "(" #argumentList ")" "\n");
#define SHIM_CALL_VOID(shimClass, functionName, shimClassFunctionName, shimObject, parameters, argumentList) \
void shimClass::functionName(parameters){ \
	 \
	if(shimDefinition->shimActive){ \
		if(!shimDefinition->Pre##shimClassFunctionName##functionName(argumentList)){ \
			return; \
		} \
	} \
	shimDefinition->shimObject->functionName(argumentList); \
	if(shimDefinition->shimActive){ \
		shimDefinition->Pos##shimClassFunctionName##functionName(argumentList); \
	} \
} \


SHIM_CALL_RETURNS(ShimTrackedDeviceDriver, Activate, TrackedDevice, trackedDevice, uint32_t unObjectId, unObjectId, vr::EVRInitError)
SHIM_CALL_VOID(ShimTrackedDeviceDriver, Deactivate, TrackedDevice, trackedDevice, , )
SHIM_CALL_VOID(ShimTrackedDeviceDriver, EnterStandby, TrackedDevice, trackedDevice, , )
SHIM_CALL_VOID(ShimTrackedDeviceDriver, DebugRequest, TrackedDevice, trackedDevice, const char *pchRequest COMMA char *pchResponseBuffer COMMA uint32_t unResponseBufferSize, pchRequest COMMA pchResponseBuffer COMMA unResponseBufferSize)
SHIM_CALL_RETURNS_NO_ARGS(ShimTrackedDeviceDriver, GetPose, TrackedDevice, trackedDevice, vr::DriverPose_t)
// SHIM_CALL_RETURNS(ShimTrackedDeviceDriver, GetComponent, TrackedDevice, trackedDevice, const char *pchComponentNameAndVersion, pchComponentNameAndVersion, void*)

SHIM_CALL_RETURNS_NO_ARGS(ShimDisplayComponent, IsDisplayOnDesktop, DisplayComponent, displayComponent, bool)
SHIM_CALL_RETURNS_NO_ARGS(ShimDisplayComponent, IsDisplayRealDisplay, DisplayComponent, displayComponent, bool)
SHIM_CALL_VOID(ShimDisplayComponent, GetRecommendedRenderTargetSize, DisplayComponent, displayComponent, uint32_t *pnWidth COMMA uint32_t *pnHeight, pnWidth COMMA pnHeight)
SHIM_CALL_VOID(ShimDisplayComponent, GetEyeOutputViewport, DisplayComponent, displayComponent, vr::EVREye eEye COMMA uint32_t *pnX COMMA uint32_t *pnY COMMA uint32_t *pnWidth COMMA uint32_t *pnHeight, eEye COMMA pnX COMMA pnY COMMA pnWidth COMMA pnHeight)
SHIM_CALL_VOID(ShimDisplayComponent, GetProjectionRaw, DisplayComponent, displayComponent, vr::EVREye eEye COMMA float *pfLeft COMMA float *pfRight COMMA float *pfTop COMMA float *pfBottom, eEye COMMA pfLeft COMMA pfRight COMMA pfTop COMMA pfBottom)
SHIM_CALL_RETURNS(ShimDisplayComponent, ComputeDistortion, DisplayComponent, displayComponent, vr::EVREye eEye COMMA float fU COMMA float fV, eEye COMMA fU COMMA fV, vr::DistortionCoordinates_t)
SHIM_CALL_RETURNS(ShimDisplayComponent, ComputeInverseDistortion, DisplayComponent, displayComponent, vr::HmdVector2_t *pResult COMMA vr::EVREye eEye COMMA uint32_t unChannel COMMA float fU COMMA float fV, pResult COMMA eEye COMMA unChannel COMMA fU COMMA fV, bool)
SHIM_CALL_VOID(ShimDisplayComponent, GetWindowBounds, DisplayComponent, displayComponent, int32_t *pnX COMMA int32_t *pnY COMMA uint32_t *pnWidth COMMA uint32_t *pnHeight, pnX COMMA pnY COMMA pnWidth COMMA pnHeight)

