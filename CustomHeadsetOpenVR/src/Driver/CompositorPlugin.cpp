#include "CompositorPlugin.h"
#include "ShaderReplacement.h"
#include "../Config/ConfigLoader.h"
#include <mutex>

bool initialized = false;
std::mutex initMutex;
ShaderReplacement shaderReplacement;

// initialize the shader replacement
void Initialize(){
	initMutex.lock();
	if(initialized){
		initMutex.unlock();
		return;
	}
	initialized = true;
	initMutex.unlock();
	
	driverConfigLoader.watchInfo = true;
	driverConfigLoader.Start();
	
	shaderReplacement.Initialize();
}

vr::EVRInitError CompositorPlugin::Init(vr::IVRDriverContext* pDriverContext){
	// initialise this driver
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	Initialize();
	return vr::VRInitError_None;
};
void CompositorPlugin::Cleanup(){
	
};
const char* const* CompositorPlugin::GetInterfaceVersions(){
	return vr::k_InterfaceVersions;
}
void* CompositorPlugin::GetComponent(const char *pchComponentNameAndVersion){
	return nullptr;
};



// I can't actually get CompositorPlugin working so just inject it into vrcompositor.exe directly


#include <thread>
#include <windows.h>
void winInit(){
	if(initialized){
		return;
	}
	
	// check that the executable we are in is vrcompositor.exe
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find(L"vrcompositor.exe");
	if (pos == std::wstring::npos){
		return;
	}
	
	Initialize();
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved){
	if(fdwReason == DLL_PROCESS_ATTACH && !initialized) {
		std::thread tr(winInit);
		tr.detach();
	}
	
	switch(fdwReason){
		case DLL_PROCESS_ATTACH:
			// code for library load
			
			break;
		case DLL_PROCESS_DETACH:
			// code for library unload
			break;
	}
	return TRUE;
}