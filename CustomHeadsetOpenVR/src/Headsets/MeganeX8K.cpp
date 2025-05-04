#include "MeganeX8K.h"
#include "../Distortion/RadialBezierDistortionProfile.h"


void MeganeX8KShim::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){
	DriverLog("PosTrackedDeviceActivate");


	// get property container
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	
	std::string modelNumber = vr::VRProperties()->GetStringProperty(container, vr::Prop_ModelNumber_String);
	if(modelNumber != "MeganeX superlight 8K"){
		// deactivate shim if this is not a MeganeX superlight 8K
		shimActive = false;
		return;
	}

	isActive = true;
	// TODO: apply a propper hidden area mesh
	vr::HmdVector2_t mesh[1];
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Left, vr::k_eHiddenAreaMesh_Standard, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, vr::k_eHiddenAreaMesh_Standard, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Left, vr::k_eHiddenAreaMesh_Inverse, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, vr::k_eHiddenAreaMesh_Inverse, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Left, vr::k_eHiddenAreaMesh_LineLoop, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, vr::k_eHiddenAreaMesh_LineLoop, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, vr::k_eHiddenAreaMesh_Max, mesh, 0);
	vr::VRHiddenArea()->SetHiddenArea(vr::Eye_Right, vr::k_eHiddenAreaMesh_Max, mesh, 0);
	
	
	// avoid "not fullscreen" warnings from vrmonitor
	vr::VRProperties()->SetBoolProperty( container, vr::Prop_IsOnDesktop_Bool, false);
	vr::VRProperties()->SetBoolProperty( container, vr::Prop_DisplayDebugMode_Bool, true);
	
	// I think this is already the default and produces true blacks
	// vr::VRProperties()->SetFloatProperty( container, vr::Prop_DisplayGCBlackClamp_Float, 0.0f);
	// vr::VRProperties()->SetFloatProperty( container, vr::Prop_DriverRequestedMuraCorrectionMode_Int32, vr::EVRMuraCorrectionMode_NoCorrection);
	
	
	// Set EDID id
	vr::VRProperties()->SetInt32Property(container, vr::Prop_EdidVendorID_Int32, 0xcc4c); // SFL megenex
	// vr::VRProperties()->EraseProperty(container, vr::Prop_EdidVendorID_Int32);
	vr::VRProperties()->EraseProperty(container, vr::Prop_EdidProductID_Int32);
	
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_EdidVendorID_Int32, 0xd222); // HVR htc vr
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_EdidProductID_Int32, 43521); // vive
	
	// DSC does not need to be set manually but these are available
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_DSCVersion_Int32, 2);
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_DSCSliceCount_Int32, 4);
	// vr::VRProperties()->SetInt32Property(container, vr::Prop_DSCBPPx16_Int32, 8 * 16);
	
	// Prop_Hmd_SupportsHDR10_Bool does not actually make the connection to the headset 10 bit, however the compositor screenshots come out in 10 bit
	// vr::VRProperties()->SetBoolProperty(container, vr::Prop_Hmd_SupportsHDR10_Bool, true);
	// vr::VRProperties()->SetBoolProperty(container, vr::Prop_Hmd_SupportsHDCP14LegacyCompat_Bool, false);
	
	// This may need to be tweaked slightly to improve tracking for quick motions
	// vr::VRProperties()->SetFloatProperty( container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.011f );
	
	
	// set ipd
	float ipd = vr::VRSettings()->GetFloat("driver_CustomHeadsetOpenVR", "ipd");
	SetIPD(ipd / 1000.0);
	
	
	// vr::VRServerDriverHost()->SetRecommendedRenderTargetSize(unObjectId, 5000, 5000);
	distortionProfile = new RadialBezierDistortionProfile();
	distortionProfile->resolution = 3552;
	distortionProfile->Initialize();
	
	// start collection of the context so we can send events later
	deviceProvider->SendContextCollectionEvents(unObjectId);
	
	testThread = std::thread( &MeganeX8KShim::TestThread, this );
	
	returnValue = vr::VRInitError_None;
}
void MeganeX8KShim::PosTrackedDeviceDeactivate(){
	isActive = false;
	DriverLog("PosTrackedDeviceDeactivate");
}

// defines the fov of the input image
bool MeganeX8KShim::PreDisplayComponentGetProjectionRaw(vr::EVREye &eEye, float *&pfLeft, float *&pfRight, float *&pfBottom, float *&pfTop){
	distortionProfile->GetProjectionRaw(eEye, pfLeft, pfRight, pfBottom, pfTop);
	return false;
}

// run for each vertex of the distortion mesh and outputs the uv coordinates to sample for each color
bool MeganeX8KShim::PreDisplayComponentComputeDistortion(vr::EVREye &eEye, float &fU, float &fV, vr::DistortionCoordinates_t &coordinates){
	// change range to -1 to 1
	fU = fU * 2.0f - 1.0f;
	fV = fV * 2.0f - 1.0f;
	if(eEye == vr::Eye_Left){
		float tmp = fU;
		fU = -fV;
		fV = tmp;
	}else{
		float tmp = fU;
		fU = fV;
		fV = -tmp;
	}
	float redV = fV;
	float greenV = fV;
	// apply sub pixel offsets for super sampling
	double subpixelOffset = 1.0f / 3.0f / 3552.0f;
	// if(testToggle){
	if(eEye == vr::Eye_Left){
		redV -= subpixelOffset;
		greenV += subpixelOffset;
	}else{
		redV += subpixelOffset;
		greenV -= subpixelOffset;
	}
	// }
	Point2D distortionRed = distortionProfile->ComputeDistortion(eEye, ColorChannelRed, fU, redV);
	Point2D distortionGreen = distortionProfile->ComputeDistortion(eEye, ColorChannelGreen, fU, greenV);
	Point2D distortionBlue = distortionProfile->ComputeDistortion(eEye, ColorChannelBlue, fU, fV);
	
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
	returnValue = false;
	return false;
};

// defined the resolution of the display or the size of the window when on the desktop
bool MeganeX8KShim::PreDisplayComponentGetWindowBounds(int32_t *&pnX, int32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){
	*pnX = 0;
	// *pnX = 3840 + 1080;
	*pnY = 0;
	// *pnWidth = 2160;
	// *pnHeight = 1200;
	// applies to direct mode as well
	*pnWidth = 7104;
	*pnHeight = 3840;
	// *pnWidth = 5328;
	// *pnHeight = 2880;
	return false;
}

// define where each eye is drawn onto the output screen
bool MeganeX8KShim::PreDisplayComponentGetEyeOutputViewport(vr::EVREye &eEye, uint32_t *&pnX, uint32_t *&pnY, uint32_t *&pnWidth, uint32_t *&pnHeight){
	if(eEye == vr::Eye_Left){
		*pnX = 0;
		// *pnY = 0;
		*pnY = (3840 - 3552) / 2;
		*pnWidth = 3552;
		// *pnHeight = 3840;
		*pnHeight = 3552;
	}else{
		*pnX = 3552;
		// *pnY = 0;
		*pnY = (3840 - 3552) / 2;
		*pnWidth = 3552;
		// *pnHeight = 3840;
		*pnHeight = 3552;
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

// this is the 100% resolution in steamvr settings
bool MeganeX8KShim::PreDisplayComponentGetRecommendedRenderTargetSize(uint32_t* &pnWidth, uint32_t* &pnHeight){
	*pnWidth = 3552;
	*pnHeight = 3552;
	// nWidth = 2880;
	// nHeight = 2880;
	return false;
}

// set ipd and adjust eye to head transform accordingly. ipd is in meters.
void MeganeX8KShim::SetIPD(float ipd){
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
	vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserIpdMeters_Float, ipd);
	vr::HmdMatrix34_t leftEye = {{
		{1, 0, 0, -ipd / 2.0f},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
	}};
	vr::HmdMatrix34_t rightEye = {{
		{1, 0, 0, ipd / 2.0f},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
	}};
	vr::VRServerDriverHost()->SetDisplayEyeToHead(0, leftEye, rightEye);
}



// a thread that is run every 5 seconds to test things with
void MeganeX8KShim::TestThread(){
	while (isActive){
		testToggle = !testToggle;
		
		// DriverLog("TestToggle: %d\n", testToggle);
		
		// uncomment this to regenerate the distortion mesh which will cause stutters
		// deviceProvider->SendVendorEvent(0, vr::VREvent_LensDistortionChanged, {}, 0);
		
		vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(0);
		
		
		std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );
	}
}