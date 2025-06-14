#include "DeviceProvider.h"
#include "../Config/ConfigLoader.h"
// #include "CompositorPlugin.h"
#include "openvr_driver.h"


#if defined( _WIN32 )
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined( __GNUC__ ) || defined( COMPILER_GCC ) || defined( __APPLE__ )
#define HMD_DLL_EXPORT extern "C" __attribute__( ( visibility( "default" ) ) )
#define HMD_DLL_IMPORT extern "C"
#else
#error "Unsupported Platform."
#endif

CustomHeadsetDeviceProvider deviceProvider = {};
// CompositorPlugin compositorPlugin = {};

// this is the main entry point from vrserver
HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode){
	// driverConfigLoader.info.debugLog += "HmdDriverFactory(" + std::string(pInterfaceName) + ")\n";
	// return (void*)1; // crash SteamVR if uncommented
	
	// return CustomHeadsetDeviceProvider
	if(0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName)){
		return &deviceProvider;
	}
	// return CompositorPlugin
	// if(0 == strcmp(vr::IVRCompositorPluginProvider_Version, pInterfaceName)){
	// 	return &compositorPlugin;
	// }
	// Otherwise tell the runtime that we don't have this interface.
	if(pReturnCode){
		*pReturnCode = vr::VRInitError_Init_InterfaceNotFound;
	}
	return NULL;
}