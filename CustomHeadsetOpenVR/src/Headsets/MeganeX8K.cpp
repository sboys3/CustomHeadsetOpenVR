#include "MeganeX8K.h"
#include <chrono>
#include <cmath>
#include "../Distortion/RadialBezierDistortionProfile.h"
#include "../Config/Config.h"
#include "../Config/ConfigLoader.h"
#include "../HiddenArea/HiddenArea.h"
#include "../Helpers/DisplayHelper.h"

constexpr float kPi{ 3.1415926535897932384626433832795028841971693993751058209749445f };

void MeganeX8KShim::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){
	DriverLog("PosTrackedDeviceActivate");


	// get property container
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	
	std::string modelNumber = vr::VRProperties()->GetStringProperty(container, vr::Prop_ModelNumber_String);
	DriverLog("headset model: %s", modelNumber);
	if(modelNumber != "MeganeX superlight 8K"){
		// deactivate shim if this is not a MeganeX superlight 8K
		shimActive = false;
		return;
	}
	driverConfigLoader.info.connectedHeadset = ConfigLoader::HeadsetType::MeganeX8K;
	
	isActive = true;

	// avoid "not fullscreen" warnings from vrmonitor
	vr::VRProperties()->SetBoolProperty( container, vr::Prop_IsOnDesktop_Bool, !driverConfig.meganeX8K.directMode);
	vr::VRProperties()->SetBoolProperty( container, vr::Prop_DisplayDebugMode_Bool, driverConfig.meganeX8K.directMode);
	
	// I think this is already the default and produces true blacks
	// vr::VRProperties()->SetFloatProperty( container, vr::Prop_DisplayGCBlackClamp_Float, 0.00f);
	// vr::VRProperties()->SetFloatProperty( container, vr::Prop_DriverRequestedMuraCorrectionMode_Int32, vr::EVRMuraCorrectionMode_NoCorrection);
	
	
	// Set EDID id
	vr::VRProperties()->SetInt32Property(container, vr::Prop_EdidVendorID_Int32, 0xcc4c); // SFL megenex
	if(!driverConfig.meganeX8K.directMode){
		vr::VRProperties()->EraseProperty(container, vr::Prop_EdidVendorID_Int32);
	}
	vr::VRProperties()->EraseProperty(container, vr::Prop_EdidProductID_Int32);
	
	// it blackscreens and immediately crashes windows when changed at runtime
	vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplaySupportsRuntimeFramerateChange_Bool, false);
	
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_EdidVendorID_Int32, 0xd222); // HVR htc vr
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_EdidProductID_Int32, 43521); // vive
	
	// DSC does not need to be set manually but these are available
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_DSCVersion_Int32, 2);
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_DSCSliceCount_Int32, 4);
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_DSCBPPx16_Int32, 8 * 16);
	
	// Prop_Hmd_SupportsHDR10_Bool does not actually make the connection to the headset 10 bit, however the compositor screenshots come out in 10 bit
	// vr::VRProperties()->SetBoolProperty(container, vr::Prop_Hmd_SupportsHDR10_Bool, true);
	// vr::VRProperties()->SetBoolProperty(container, vr::Prop_Hmd_SupportsHDCP14LegacyCompat_Bool, false);
	
	
	
	// set ipd
	// float ipd = vr::VRSettings()->GetFloat("driver_CustomHeadsetOpenVR", "ipd");
	// SetIPD(ipd / 1000.0);
	
	
	// vr::VRServerDriverHost()->SetRecommendedRenderTargetSize(unObjectId, 5000, 5000);
	distortionProfileConstructor.distortionSettings.resolution = (float)std::min(driverConfig.meganeX8K.resolutionX, driverConfig.meganeX8K.resolutionY);
	distortionProfileConstructor.distortionSettings.resolutionX = (float)driverConfig.meganeX8K.resolutionX;
	distortionProfileConstructor.distortionSettings.resolutionY = (float)driverConfig.meganeX8K.resolutionY;
	
	// initialize random value for session
	fovBurnInOffset = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 10000) / 10000.f * 3.f;
	DriverLog("fovBurnInOffset: %f", fovBurnInOffset);
	
	
	distortionProfileConstructor.distortionSettings.noneDistortionFovHorizontal = 95;
	distortionProfileConstructor.distortionSettings.noneDistortionFovVertical = 95;
	
	// start collection of the context so we can send events later
	deviceProvider->SendContextCollectionEvents(unObjectId);
	
	if(!testThread.joinable()){
		testThread = std::thread(&MeganeX8KShim::TestThread, this);
	}
	
	UpdateSettings();
	
	// returnValue = vr::VRInitError_None;
}
void MeganeX8KShim::PosTrackedDeviceDeactivate(){
	isActive = false;
	DriverLog("PosTrackedDeviceDeactivate");
}

// defines the fov of the input image
bool MeganeX8KShim::PreDisplayComponentGetProjectionRaw(vr::EVREye &eEye, float *&pfLeft, float *&pfRight, float *&pfBottom, float *&pfTop){
	distortionProfileConstructor.profile->GetProjectionRaw(eEye, pfLeft, pfRight, pfBottom, pfTop);
	DriverLog("PreDisplayComponentGetProjectionRaw %f %f %f %f", *pfLeft, *pfRight, *pfBottom, *pfTop);
	if(eEye == vr::Eye_Left && driverConfig.meganeX8K.disableEye & 1 && driverConfig.meganeX8K.disableEyeDecreaseFov){
		*pfTop = *pfRight = 0.000001f;
		*pfBottom = *pfLeft = -*pfTop;
	}
	if(eEye == vr::Eye_Right && driverConfig.meganeX8K.disableEye & 2 && driverConfig.meganeX8K.disableEyeDecreaseFov){
		*pfTop = *pfRight = 0.000001f;
		*pfBottom = *pfLeft = -*pfTop;
	}	
	return false;
}

// run for each vertex of the distortion mesh and outputs the uv coordinates to sample for each color
bool MeganeX8KShim::PreDisplayComponentComputeDistortion(vr::EVREye &eEye, float &fU, float &fV, vr::DistortionCoordinates_t &coordinates){

	float minResolution = (float)std::min(driverConfig.meganeX8K.resolutionX, driverConfig.meganeX8K.resolutionY);
	// change range to -1 to 1 for coverage of the minResolution square
	fU = (fU - 0.5f) * 2.0f * driverConfig.meganeX8K.resolutionY / minResolution;
	fV = (fV - 0.5f) * 2.0f * driverConfig.meganeX8K.resolutionX / minResolution;
	if(eEye == vr::Eye_Left){
		float tmp = fU;
		fU = -fV;
		fV = tmp;
	}else{
		float tmp = fU;
		fU = fV;
		fV = -tmp;
	}
	
	fU /= (float)driverConfig.meganeX8K.distortionZoom;
	fV /= (float)driverConfig.meganeX8K.distortionZoom;
	
	float redV = fV;
	float greenV = fV;
	
	// apply sub pixel offsets for super sampling
	// the resolution is hardcoded here because the physical pixel sizes remain constant regardless of resolution
	float subpixelOffset = (float)(driverConfig.meganeX8K.subpixelShift / 3552);
	// if(testToggle){
	if(eEye == vr::Eye_Left){
		redV -= subpixelOffset;
		greenV += subpixelOffset;
	}else{
		redV += subpixelOffset;
		greenV -= subpixelOffset;
	}
	// }
	
	std::lock_guard<std::mutex> lock(distortionProfileLock);
	// apply distortion profile to each color channel
	Point2D distortionRed = distortionProfileConstructor.profile->ComputeDistortion(eEye, ColorChannelRed, fU, redV);
	Point2D distortionGreen = distortionProfileConstructor.profile->ComputeDistortion(eEye, ColorChannelGreen, fU, greenV);
	Point2D distortionBlue = distortionProfileConstructor.profile->ComputeDistortion(eEye, ColorChannelBlue, fU, fV);
	
	if(eEye == vr::Eye_Left && driverConfig.meganeX8K.disableEye & 1){
		// this will completely cull the render mesh
		distortionRed = distortionGreen = distortionBlue = {-1, -1};
	}
	if(eEye == vr::Eye_Right && driverConfig.meganeX8K.disableEye & 2){
		distortionRed = distortionGreen = distortionBlue = {-1, -1};
	}
	
	coordinates.rfRed[0] = distortionRed.x;
	coordinates.rfRed[1] = distortionRed.y;
	coordinates.rfGreen[0] = distortionGreen.x;
	coordinates.rfGreen[1] = distortionGreen.y;
	coordinates.rfBlue[0] = distortionBlue.x;
	coordinates.rfBlue[1] = distortionBlue.y;
	// change range to 0 to 1
	coordinates.rfRed[0] = coordinates.rfRed[0] * 0.5f + 0.5f;
	coordinates.rfRed[1] = coordinates.rfRed[1] * 0.5f + 0.5f;
	coordinates.rfGreen[0] = coordinates.rfGreen[0] * 0.5f + 0.5f;
	coordinates.rfGreen[1] = coordinates.rfGreen[1] * 0.5f + 0.5f;
	coordinates.rfBlue[0] = coordinates.rfBlue[0] * 0.5f + 0.5f;
	coordinates.rfBlue[1] = coordinates.rfBlue[1] * 0.5f + 0.5f;
	return false;
}

bool MeganeX8KShim::PreDisplayComponentIsDisplayOnDesktop(bool &returnValue){
	returnValue = true;
	return false;
};
bool MeganeX8KShim::PreDisplayComponentIsDisplayRealDisplay(bool &returnValue){
	returnValue = !driverConfig.meganeX8K.directMode;
	return false;
};

// defined the resolution of the display or the size of the window when on the desktop
bool MeganeX8KShim::PreDisplayComponentGetWindowBounds(int32_t *&pnX, int32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){
	*pnX = 0;
	// *pnX = 3840 + 1080;
	// *pnX = 3840 + 1080 + 1920;
	*pnY = 0;
	// *pnWidth = 2160;
	// *pnHeight = 1200;
	// applies to direct mode as well
	*pnWidth = driverConfig.meganeX8K.resolutionY * 2;
	*pnHeight = driverConfig.meganeX8K.resolutionX;
	// *pnWidth = 5328;
	// *pnHeight = 2880;
	if(!driverConfig.meganeX8K.directMode){
		int edidVendorId = vr::VRProperties()->GetInt32Property(vr::VRProperties()->TrackedDeviceToPropertyContainer(0), vr::Prop_EdidVendorID_Int32);
		int edidProductId = vr::VRProperties()->GetInt32Property(vr::VRProperties()->TrackedDeviceToPropertyContainer(0), vr::Prop_EdidProductID_Int32);
		FindDisplayPosition(*pnWidth, *pnHeight, edidVendorId, edidProductId, pnX, pnY);
	}
	return false;
}

// define where each eye is drawn onto the output screen
bool MeganeX8KShim::PreDisplayComponentGetEyeOutputViewport(vr::EVREye &eEye, uint32_t *&pnX, uint32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){
	// X and Y from teh config are seemingly reversed here because the panels are rotated 90 degrees
	if(eEye == vr::Eye_Left){
		*pnX = 0;
		*pnY = 0;
		// *pnY = (3840 - 3552) / 2;
		*pnWidth = driverConfig.meganeX8K.resolutionY;
		*pnHeight = driverConfig.meganeX8K.resolutionX;
		// *pnHeight = 3552;
	}else{
		*pnX = driverConfig.meganeX8K.resolutionY;
		*pnY = 0;
		// *pnY = (3840 - 3552) / 2;
		*pnWidth = driverConfig.meganeX8K.resolutionY;
		*pnHeight = driverConfig.meganeX8K.resolutionX;
		// *pnHeight = 3552;
	}
	// if(eEye == vr::Eye_Left){
	// 	*pnX = 0;
	// 	*pnY = (5328 - 2880) / 2;;
	// 	*pnWidth = 2880;
	// 	*pnHeight = 2880;
	// }else{
	// 	*pnX = 2880;
	// 	*pnY = (5328 - 2880) / 2;;
	// 	*pnWidth = 2880;
	// 	*pnHeight = 2880;
	// }
	return false;
};

void MeganeX8KShim::GetRecommendedRenderTargetSize(uint32_t* renderWidth, uint32_t* renderHeight){
	distortionProfileConstructor.GetRecommendedRenderTargetSize(renderWidth, renderHeight);
	*renderWidth  = static_cast<uint32_t>(*renderWidth  * driverConfig.meganeX8K.renderResolutionMultiplierX);
	*renderHeight = static_cast<uint32_t>(*renderHeight * driverConfig.meganeX8K.renderResolutionMultiplierY);
	// save to info
	float projectionLeft, projectionRight, projectionTop, projectionBottom;
	distortionProfileConstructor.profile->GetProjectionRaw(vr::Eye_Left, &projectionLeft, &projectionRight, &projectionBottom, &projectionTop);
	double fovX = (std::atan(projectionRight) - std::atan(projectionLeft)) * 180.0 / kPi;
	double fovY = (std::atan(projectionTop) - std::atan(projectionBottom)) * 180.0 / kPi;
	driverConfigLoader.info.renderFovX = fovX;
	driverConfigLoader.info.renderFovY = fovY;
	driverConfigLoader.info.renderResolution100PercentX = *renderWidth;
	driverConfigLoader.info.renderResolution100PercentY = *renderHeight;
	distortionProfileConstructor.profile->GetRecommendedRenderTargetSize(&driverConfigLoader.info.renderResolution1To1X, &driverConfigLoader.info.renderResolution1To1Y);
	double requiredPercentX = (double)(driverConfigLoader.info.renderResolution1To1X * driverConfigLoader.info.renderResolution1To1X) / 
		(double)(driverConfigLoader.info.renderResolution100PercentX * driverConfigLoader.info.renderResolution100PercentX);
	double requiredPercentY = (double)(driverConfigLoader.info.renderResolution1To1Y * driverConfigLoader.info.renderResolution1To1Y) / 
		(double)(driverConfigLoader.info.renderResolution100PercentY * driverConfigLoader.info.renderResolution100PercentY);
	double requiredPercent = std::max(requiredPercentX, requiredPercentY);
	driverConfigLoader.info.renderResolution1To1Percent = requiredPercent * 100.0;
	driverConfigLoader.WriteInfo();
	
	// write advanced super sampling resolution, this won't apply until the game is relaunched
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
	vr::VRProperties()->SetInt32Property(container, vr::Prop_Hmd_MaxDistortedTextureWidth_Int32, (int)(driverConfigLoader.info.renderResolution1To1X * std::sqrt(driverConfig.meganeX8K.superSamplingFilterPercent / 100.0)));
	vr::VRProperties()->SetInt32Property(container, vr::Prop_Hmd_MaxDistortedTextureHeight_Int32, (int)(driverConfigLoader.info.renderResolution1To1Y * std::sqrt(driverConfig.meganeX8K.superSamplingFilterPercent / 100.0)));
}

// this is the 100% resolution in steamvr settings
bool MeganeX8KShim::PreDisplayComponentGetRecommendedRenderTargetSize(uint32_t* &pnWidth, uint32_t* &pnHeight){
	GetRecommendedRenderTargetSize(pnWidth, pnHeight);
	return false;
}

// set ipd and adjust eye to head transform accordingly. ipd is in meters. angle it in radians
void MeganeX8KShim::SetIPD(float ipd, float angle){
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
	vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserIpdMeters_Float, ipd);
	float c = std::cos(angle);
	float s = std::sin(angle);
	vr::HmdMatrix34_t leftEye = {{
		{ c, 0, s, -ipd / 2.0f},
		{ 0, 1, 0, 0},
		{-s, 0, c, 0},
	}};
	vr::HmdMatrix34_t rightEye = {{
		{c, 0, -s, ipd / 2.0f},
		{0, 1,  0, 0},
		{s, 0,  c, 0},
	}};
	vr::VRServerDriverHost()->SetDisplayEyeToHead(0, leftEye, rightEye);
}


void MeganeX8KShim::RunFrame(){
	double now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000000.0;
	double frameTime = now - lastFrameTime;
	if(lastFrameTime == 0){
		lastFrameTime = now;
		frameTime = 0;
	}
	static int debugFrameCount = 0;
	debugFrameCount++;
	if(frameTime >= 1.0 / 100.0){
		// only run a maximum of 100 times a second
		
		vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
	
		if(driverConfig.meganeX8K.stationaryDimming.enable){
			vr::TrackedDevicePose_t hmdPos;
			vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0, &hmdPos, 1);
			vr::HmdMatrix34_t rotation = hmdPos.mDeviceToAbsoluteTracking;
			// calculate angle between last and current rotation acos((trace(A'*B)-1)/2)*360/2/pi
			double angle = std::acos((rotation.m[0][0] * lastMovementRotation.m[0][0] + rotation.m[1][0] * lastMovementRotation.m[1][0] + rotation.m[2][0] * lastMovementRotation.m[2][0] +
				rotation.m[0][1] * lastMovementRotation.m[0][1] + rotation.m[1][1] * lastMovementRotation.m[1][1] + rotation.m[2][1] * lastMovementRotation.m[2][1] +
				rotation.m[0][2] * lastMovementRotation.m[0][2] + rotation.m[1][2] * lastMovementRotation.m[1][2] + rotation.m[2][2] * lastMovementRotation.m[2][2] - 1) / 2.0) * 360.0 / 2.0 / kPi;
			if(angle > driverConfig.meganeX8K.stationaryDimming.movementThreshold){
				// moved past threshold
				// DriverLog("angle: %f", angle);
				lastMovementRotation = rotation;
				lastMovementTime = now;
			}
			double dimmingAmount = 1 - driverConfig.meganeX8K.stationaryDimming.dimBrightnessPercent / 100.0;
			if(lastMovementTime > now - driverConfig.meganeX8K.stationaryDimming.movementTime){
				// still moving
				dimmingMultiplier += dimmingAmount / driverConfig.meganeX8K.stationaryDimming.brightenSeconds * frameTime;
			}else{
				// stationary
				dimmingMultiplier -= dimmingAmount / driverConfig.meganeX8K.stationaryDimming.dimSeconds * frameTime;
			}
			dimmingMultiplier = std::max(driverConfig.meganeX8K.stationaryDimming.dimBrightnessPercent / 100.0, std::min(1.0, dimmingMultiplier));
		}else{
			dimmingMultiplier = 1;
		}
		
		Config::Color color = driverConfig.meganeX8K.colorMultiplier;
		double brightness = 1;
		// brightness *= std::sin(now) * 0.5 + 0.5;
		brightness *= dimmingMultiplier;
		color.r *= brightness;
		color.g *= brightness;
		color.b *= brightness;
		if(lastColorMultiplier.r != color.r || lastColorMultiplier.g != color.g || lastColorMultiplier.b != color.b){
			lastColorMultiplier = color;
			vr::VRProperties()->SetVec3Property(container, vr::Prop_DisplayColorMultLeft_Vector3, {(float)color.r, (float)color.g, (float)color.b});
			vr::VRProperties()->SetVec3Property(container, vr::Prop_DisplayColorMultRight_Vector3, {(float)color.r, (float)color.g, (float)color.b});
		}
		
		
		lastFrameTime = now;
	}
	
	

	
	
	
	
	if(driverConfig.hasBeenUpdated || ((now - lastDistortionChangeTime) > 0.5 && needsDistortionFinalization)){
		UpdateSettings();
	}
}

void MeganeX8KShim::UpdateSettings(){
	double now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000000.0;
	
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
	
	SetIPD((float)(driverConfig.meganeX8K.ipd + driverConfig.meganeX8K.ipdOffset) / 1000.f, (float)(driverConfig.meganeX8K.eyeRotation * kPi / 180.0f));

	vr::VRProperties()->SetFloatProperty(container, vr::Prop_DisplayGCBlackClamp_Float, (float)driverConfig.meganeX8K.blackLevel);
	vr::VRProperties()->SetFloatProperty(container, vr::Prop_SecondsFromVsyncToPhotons_Float, (float)driverConfig.meganeX8K.secondsFromVsyncToPhotons);
	vr::VRProperties()->SetFloatProperty(container, vr::Prop_SecondsFromPhotonsToVblank_Float, (float)driverConfig.meganeX8K.secondsFromPhotonsToVblank);
	
	//bluetoothDevice
	if(driverConfig.meganeX8K.bluetoothDevice == 1){
		vr::VRProperties()->SetUint64Property(container, vr::Prop_AdditionalRadioFeatures_Uint64, vr::AdditionalRadioFeatures_HTCLinkBox);
	}

	if (driverConfig.meganeX8K.hiddenArea != driverConfigOld.meganeX8K.hiddenArea || driverConfigOld.meganeX8K.disableEye != driverConfig.meganeX8K.disableEye) { // This generally requires that you restart your game for it to update
		for (auto meshType : { vr::k_eHiddenAreaMesh_Standard, vr::k_eHiddenAreaMesh_Inverse, vr::k_eHiddenAreaMesh_LineLoop }) {
			vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Left,  meshType, nullptr, 0);
			vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, meshType, nullptr, 0);
		}
		// The compositor uses the LineLoop/Inverse mesh (depending on which is available),
		// but I haven't been able to set those without causing issues for the compositor at boot.
		// So for now we just set the Standard mesh that games seem to use (it's the one that matters for performance).
		if (const auto& haConf = driverConfig.meganeX8K.hiddenArea; haConf.enable) {
			for (auto meshType : { vr::k_eHiddenAreaMesh_Standard }) {
				for (auto eye : { vr::Eye_Left, vr::Eye_Right}) {
					auto mesh = HiddenArea::CreateHiddenAreaMesh(eye, meshType, haConf);
					auto err = vr::VRHiddenArea()->SetHiddenArea(eye, meshType, mesh.data(), (uint32_t)mesh.size());
					if (err != vr::TrackedProp_Success) {
						DriverLog("Failed to setHiddenArea type %d with error %d", static_cast<int>(meshType), err);
					}
				}
			}
		}
		if(driverConfig.meganeX8K.disableEye & 1){
			vr::HmdVector2_t coverMesh[] = {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}}; // cover the whole screen
			vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Left, vr::k_eHiddenAreaMesh_Standard, coverMesh, 6);
		}
		if(driverConfig.meganeX8K.disableEye & 2){
			vr::HmdVector2_t coverMesh[] = {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}}; // cover the whole screen
			vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, vr::k_eHiddenAreaMesh_Standard, coverMesh, 6);
		}
	}

	
	distortionProfileConstructor.distortionSettings.maxFovX = (float)driverConfig.meganeX8K.maxFovX;
	distortionProfileConstructor.distortionSettings.maxFovY = (float)driverConfig.meganeX8K.maxFovY;
	distortionProfileConstructor.distortionSettings.fovZoom = (float)driverConfig.meganeX8K.fovZoom;
	if(driverConfig.meganeX8K.fovBurnInPrevention){
		distortionProfileConstructor.distortionSettings.maxFovX += fovBurnInOffset;
		distortionProfileConstructor.distortionSettings.maxFovY += fovBurnInOffset;
	}
	
	
	bool shouldReInitializeDistortion = false;
	shouldReInitializeDistortion |= driverConfigOld.meganeX8K.maxFovX != driverConfig.meganeX8K.maxFovX;
	shouldReInitializeDistortion |= driverConfigOld.meganeX8K.maxFovY != driverConfig.meganeX8K.maxFovY;
	shouldReInitializeDistortion |= driverConfigOld.meganeX8K.fovZoom != driverConfig.meganeX8K.fovZoom;
	shouldReInitializeDistortion |= driverConfigOld.meganeX8K.renderResolutionMultiplierX != driverConfig.meganeX8K.renderResolutionMultiplierX;
	shouldReInitializeDistortion |= driverConfigOld.meganeX8K.renderResolutionMultiplierY != driverConfig.meganeX8K.renderResolutionMultiplierY;
	shouldReInitializeDistortion |= driverConfigOld.meganeX8K.fovBurnInPrevention != driverConfig.meganeX8K.fovBurnInPrevention;
	
	std::lock_guard<std::mutex> lock(distortionProfileLock);
	bool loadedNewDistortionProfile = distortionProfileConstructor.LoadDistortionProfile(driverConfig.meganeX8K.distortionProfile);
	bool shouldUpdateDistortion = loadedNewDistortionProfile || shouldReInitializeDistortion;
	shouldUpdateDistortion |= driverConfigOld.meganeX8K.distortionZoom != driverConfig.meganeX8K.distortionZoom;
	shouldUpdateDistortion |= driverConfigOld.meganeX8K.subpixelShift != driverConfig.meganeX8K.subpixelShift;
	shouldUpdateDistortion |= driverConfigOld.meganeX8K.distortionMeshResolution != driverConfig.meganeX8K.distortionMeshResolution;
	shouldUpdateDistortion |= driverConfigOld.meganeX8K.disableEye != driverConfig.meganeX8K.disableEye;
	shouldUpdateDistortion |= driverConfigOld.meganeX8K.disableEyeDecreaseFov != driverConfig.meganeX8K.disableEyeDecreaseFov;
	shouldUpdateDistortion |= (now - lastDistortionChangeTime) > 0.5 && needsDistortionFinalization;

	
	vr::VRProperties()->SetInt32Property(container, vr::Prop_DistortionMeshResolution_Int32, std::min(1024, driverConfig.meganeX8K.distortionMeshResolution));
	if(driverConfigOld.meganeX8K.superSamplingFilterPercent != driverConfig.meganeX8K.superSamplingFilterPercent){
		uint32_t renderResolutionX, renderResolutionY;
		GetRecommendedRenderTargetSize(&renderResolutionX, &renderResolutionY);
	}
	
	if(shouldReInitializeDistortion && !loadedNewDistortionProfile){
		distortionProfileConstructor.ReInitializeProfile();
	}
	
	if(shouldUpdateDistortion){
		// mark to finalize if regenerating within 0.5 seconds of the last
		needsDistortionFinalization = now - lastDistortionChangeTime < 0.5;
		lastDistortionChangeTime = now;
		// it has changed so signal the compositor to regenerate the distortion mesh
		deviceProvider->SendVendorEvent(0, vr::VREvent_LensDistortionChanged, {}, 0);
		// also update fov
		float leftEyeLeft, leftEyeRight, leftEyeTop, leftEyeBottom;
		distortionProfileConstructor.profile->GetProjectionRaw(vr::Eye_Left, &leftEyeLeft, &leftEyeRight, &leftEyeBottom, &leftEyeTop);
		float rightEyeLeft, rightEyeRight, rightEyeTop, rightEyeBottom;
		distortionProfileConstructor.profile->GetProjectionRaw(vr::Eye_Right, &rightEyeLeft, &rightEyeRight, &rightEyeBottom, &rightEyeTop);
		if(driverConfig.meganeX8K.disableEye & 1 && driverConfig.meganeX8K.disableEyeDecreaseFov){
			leftEyeRight = leftEyeTop = 0.000001f;
			leftEyeBottom = leftEyeLeft = -leftEyeTop;
		}
		if(driverConfig.meganeX8K.disableEye & 2 && driverConfig.meganeX8K.disableEyeDecreaseFov){
			rightEyeLeft = rightEyeTop = 0.000001f;
			rightEyeBottom = rightEyeRight = -rightEyeTop;
		}
		vr::VRServerDriverHost()->SetDisplayProjectionRaw(0, vr::HmdRect2_t{{leftEyeLeft, leftEyeBottom}, {leftEyeRight, leftEyeTop}}, vr::HmdRect2_t{{rightEyeLeft, rightEyeBottom}, {rightEyeRight, rightEyeTop}});
		// this requires reallocations so only do it when a finalization is not queued
		if(!needsDistortionFinalization){
			// get max fov from the new profile
			DistortionProfileConstructor maxFovConstructor = DistortionProfileConstructor();
			maxFovConstructor.distortionSettings = distortionProfileConstructor.distortionSettings;
			maxFovConstructor.distortionSettings.maxFovX = 170;
			maxFovConstructor.distortionSettings.maxFovY = 170;
			maxFovConstructor.LoadDistortionProfile(driverConfig.meganeX8K.distortionProfile);
			maxFovConstructor.profile->GetProjectionRaw(vr::Eye_Left, &leftEyeLeft, &leftEyeRight, &leftEyeBottom, &leftEyeTop);
			driverConfigLoader.info.renderFovMaxX = (std::atan(leftEyeRight) - std::atan(leftEyeLeft)) * 180.0 / kPi;
			driverConfigLoader.info.renderFovMaxY = (std::atan(leftEyeTop) - std::atan(leftEyeBottom)) * 180.0 / kPi;
			// update render target size
			uint32_t renderResolutionX, renderResolutionY;
			GetRecommendedRenderTargetSize(&renderResolutionX, &renderResolutionY);
			vr::VRServerDriverHost()->SetRecommendedRenderTargetSize(0, renderResolutionX, renderResolutionY);
		}
	}
}


// a thread that is run every 5 seconds to test things with
void MeganeX8KShim::TestThread(){
	while (isActive){
		testToggle = !testToggle;
		
		// DriverLog("TestToggle: %d\n", testToggle);
		
		
		// uncomment this to regenerate the distortion mesh which will cause stutters
		// deviceProvider->SendVendorEvent(0, vr::VREvent_LensDistortionChanged, {}, 0);
		
		vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
		
		
		// vr::VRProperties()->SetFloatProperty( container, vr::Prop_DisplayGCBlackClamp_Float, testToggle ? 0.00f : 0.02f);
		
		// float brightness = std::sin(now) * 0.5 + 0.5;
		// vr::VRProperties()->SetVec3Property(container, vr::Prop_DisplayColorMultLeft_Vector3, {brightness, brightness, brightness});
		// vr::VRProperties()->SetVec3Property(container, vr::Prop_DisplayColorMultRight_Vector3, {brightness, brightness, brightness});
		
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
}