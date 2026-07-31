#include "AAPVRBlocker.h"

#ifdef _WIN32

#include "PimaxCommon.h"
#include "../Driver/DriverLog.h"
#include "../Config/ConfigLoader.h"


#ifdef PVR_EXISTS
	#include "C:/Program Files/Pimax/Sdk/Include/PVR_API.h"
#endif

#include <windows.h>
#include <string>

#include "../../../ThirdParty/minhook/include/MinHook.h"
#undef max
#undef min

static bool sHasHooked = false;
static HMODULE sLighthouseHandle = nullptr;

static bool sShouldBlockCached = false;
static bool sShouldBlockValid = false;

static void* (*sOrigLoadLibraryA)(const char* lpLibFileName) = nullptr;
static void* (*sOrigLoadLibraryW)(const wchar_t* lpLibFileName) = nullptr;
static void* (*sOrigLoadLibraryExA)(const char* lpLibFileName, void* hFile, unsigned long dwFlags) = nullptr;
static void* (*sOrigLoadLibraryExW)(const wchar_t* lpLibFileName, void* hFile, unsigned long dwFlags) = nullptr;

static bool IsLighthouseDriverPath(const char* path) {
	if (!path) return false;
	std::string lowerPath(path);
	for (auto& c : lowerPath) c = tolower(c);
	return lowerPath.find("driver_lighthouse") != std::string::npos;
}

static bool IsLighthouseDriverPath(const wchar_t* path) {
	if (!path) return false;
	return wcsstr(path, L"driver_lighthouse") != nullptr;
}

static std::string WcharToAscii(const wchar_t* w) {
	std::string out;
	while (*w) {
		out += (char)(*w & 0xFF);
		w++;
	}
	return out;
}

static std::string GetDllName(const char* path) {
	if (!path) return "";
	std::string s(path);
	size_t lastSlash = s.find_last_of("/\\");
	return (lastSlash != std::string::npos) ? s.substr(lastSlash + 1) : s;
}

static std::string GetDllName(const wchar_t* path) {
	if (!path) return "";
	std::wstring s(path);
	size_t lastSlash = s.find_last_of(L"/\\");
	return WcharToAscii(s.c_str() + ((lastSlash != std::wstring::npos) ? lastSlash + 1 : 0));
}

static bool ContainsInsensitive(const std::string& haystack, const std::string& needle) {
	std::string lowerHay = haystack;
	for (auto& c : lowerHay) c = tolower(c);
	std::string lowerNeedle = needle;
	for (auto& c : lowerNeedle) c = tolower(c);
	return lowerHay.find(lowerNeedle) != std::string::npos;
}

static void* BLOCK_SENTINEL = (void*)1;

static void* BeforeLibraryLoad(const std::string& dllName) {
	// Return cached handle to intercept, BLOCK_SENTINEL to force nullptr, or nullptr to pass through.
	#ifdef FULLY_BLOCK_AAPVR
	// Partially blocking the AAPVR driver causes some problems, so instead just block it entirely.
	if (ContainsInsensitive(dllName, "driver_aapvr")) {
		DriverLog("AAPVRBlocker: Blocking AAPVR driver entirely");
		// AAPVRLighthouseUnblockerRemoveHooks();
		return BLOCK_SENTINEL;
	}
	#endif
	// The AAPVR driver loads after us and wraps our hook, so this never actually succeeds.
	// if (sLighthouseHandle && ContainsInsensitive(dllName, "driver_lighthouse")) {
	// 	DriverLog("AAPVRBlocker: Intercepted for lighthouse driver");
	// 	return sLighthouseHandle;
	// }
	return nullptr;
}

static void AfterLibraryLoad(const std::string& dllName, void* result) {
	// Instead unload the hooks as soon as the AAPVR driver loads, which will destroy its hook as well.
	// When the aapvr driver is fully blocked, this will not be reached.
	if (ContainsInsensitive(dllName, "driver_aapvr")) {
		DriverLog("AAPVRBlocker: AAPVR driver detected, unhooking");
		// Sacrifice my hooks to bring down the aapvr hook.
		AAPVRLighthouseUnblockerRemoveHooks();
		// AAPVRLighthouseUnblockerInjectHooks();
	}
}

static void* HookedLoadLibraryA(const char* lpLibFileName) {
	// DriverLog("AAPVRBlocker: LoadLibraryA(%s)", lpLibFileName);
	void* intercepted = BeforeLibraryLoad(GetDllName(lpLibFileName));
	if (intercepted == BLOCK_SENTINEL) return nullptr;
	if (intercepted) return intercepted;
	void* result = sOrigLoadLibraryA(lpLibFileName);
	AfterLibraryLoad(GetDllName(lpLibFileName), result);
	return result;
}

static void* HookedLoadLibraryW(const wchar_t* lpLibFileName) {
	// DriverLog("AAPVRBlocker: LoadLibraryW(%s)", WcharToAscii(lpLibFileName).c_str());
	std::string name = GetDllName(lpLibFileName);
	void* intercepted = BeforeLibraryLoad(name);
	if (intercepted == BLOCK_SENTINEL) return nullptr;
	if (intercepted) return intercepted;
	void* result = sOrigLoadLibraryW(lpLibFileName);
	AfterLibraryLoad(name, result);
	return result;
}

static void* HookedLoadLibraryExA(const char* lpLibFileName, void* hFile, unsigned long dwFlags) {
	// DriverLog("AAPVRBlocker: LoadLibraryExA(%s)", lpLibFileName);
	void* intercepted = BeforeLibraryLoad(GetDllName(lpLibFileName));
	if (intercepted == BLOCK_SENTINEL) return nullptr;
	if (intercepted) return intercepted;
	void* result = sOrigLoadLibraryExA(lpLibFileName, hFile, dwFlags);
	AfterLibraryLoad(GetDllName(lpLibFileName), result);
	return result;
}

static void* HookedLoadLibraryExW(const wchar_t* lpLibFileName, void* hFile, unsigned long dwFlags) {
	// DriverLog("AAPVRBlocker: LoadLibraryExW(%s)", WcharToAscii(lpLibFileName).c_str());
	std::string name = GetDllName(lpLibFileName);
	void* intercepted = BeforeLibraryLoad(name);
	if (intercepted == BLOCK_SENTINEL) return nullptr;
	if (intercepted) return intercepted;
	void* result = sOrigLoadLibraryExW(lpLibFileName, hFile, dwFlags);
	AfterLibraryLoad(name, result);
	return result;
}

void AAPVRLighthouseUnblockerLoadDriver() {
	if (sLighthouseHandle) {
		return;
	}

	std::string lighthousePath = driverConfigLoader.info.steamvrResources + "../drivers/lighthouse/bin/win64/driver_lighthouse.dll";

	char szPath[MAX_PATH];
	if (GetFullPathNameA(lighthousePath.c_str(), MAX_PATH, szPath, NULL) == 0) {
		DriverLog("AAPVRBlocker: Failed to resolve path for driver_lighthouse.dll");
		return;
	}
	lighthousePath = szPath;

	DriverLog("AAPVRBlocker: Loading lighthouse driver from: %s", lighthousePath.c_str());

	sLighthouseHandle = LoadLibraryExA(lighthousePath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	if (sLighthouseHandle) {
		DriverLog("AAPVRBlocker: Successfully acquired handle for driver_lighthouse.dll");
	} else {
		DriverLog("AAPVRBlocker: Failed to load driver_lighthouse.dll");
	}
}

void AAPVRLighthouseUnblockerInjectHooks() {
	if (sHasHooked) {
		return;
	}

	if (!sLighthouseHandle) {
		AAPVRLighthouseUnblockerLoadDriver();
	}

	if (!sLighthouseHandle) {
		return;
	}

	auto err = MH_Initialize();
	if (err != MH_OK && err != MH_ERROR_ALREADY_INITIALIZED) {
		DriverLog("AAPVRBlocker: Failed to initialize MinHook");
		return;
	}

	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (!hKernel32) {
		DriverLog("AAPVRBlocker: Failed to get handle for kernel32.dll");
		return;
	}

	auto result = MH_CreateHook((void*)GetProcAddress(hKernel32, "LoadLibraryA"), &HookedLoadLibraryA, (void**)&sOrigLoadLibraryA);
	if (result == MH_OK) {
		MH_EnableHook((void*)GetProcAddress(hKernel32, "LoadLibraryA"));
	} else {
		DriverLog("AAPVRBlocker: Failed to create hook for LoadLibraryA");
		return;
	}

	result = MH_CreateHook((void*)GetProcAddress(hKernel32, "LoadLibraryW"), &HookedLoadLibraryW, (void**)&sOrigLoadLibraryW);
	if (result == MH_OK) {
		MH_EnableHook((void*)GetProcAddress(hKernel32, "LoadLibraryW"));
	} else {
		DriverLog("AAPVRBlocker: Failed to create hook for LoadLibraryW");
		return;
	}

	result = MH_CreateHook((void*)GetProcAddress(hKernel32, "LoadLibraryExA"), &HookedLoadLibraryExA, (void**)&sOrigLoadLibraryExA);
	if (result == MH_OK) {
		MH_EnableHook((void*)GetProcAddress(hKernel32, "LoadLibraryExA"));
	} else {
		DriverLog("AAPVRBlocker: Failed to create hook for LoadLibraryExA");
		return;
	}

	result = MH_CreateHook((void*)GetProcAddress(hKernel32, "LoadLibraryExW"), &HookedLoadLibraryExW, (void**)&sOrigLoadLibraryExW);
	if (result == MH_OK) {
		MH_EnableHook((void*)GetProcAddress(hKernel32, "LoadLibraryExW"));
	} else {
		DriverLog("AAPVRBlocker: Failed to create hook for LoadLibraryExW");
		return;
	}

	sHasHooked = true;
	DriverLog("AAPVRBlocker: LoadLibrary hooks enabled");
}

void AAPVRLighthouseUnblockerRemoveHooks() {
	if (!sHasHooked) {
		return;
	}

	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (hKernel32) {
		MH_DisableHook(GetProcAddress(hKernel32, "LoadLibraryA"));
		MH_DisableHook(GetProcAddress(hKernel32, "LoadLibraryW"));
		MH_DisableHook(GetProcAddress(hKernel32, "LoadLibraryExA"));
		MH_DisableHook(GetProcAddress(hKernel32, "LoadLibraryExW"));
	}

	sHasHooked = false;
	DriverLog("AAPVRBlocker: LoadLibrary hooks removed");
}



bool AAPVRShouldBlock() {
	if (sShouldBlockValid) {
		return sShouldBlockCached;
	}

#ifdef PVR_EXISTS

	int noRender = pvr_getIntConfig(PimaxCommon::GetPvrSession(true), "no_render", 0);
	sShouldBlockCached = (noRender == 1);
	sShouldBlockValid = true;

	DriverLog("AAPVRBlocker: PVR no_render=%d, caching block=%d", noRender, sShouldBlockCached);
#else
	sShouldBlockCached = false;
	sShouldBlockValid = true;
	DriverLog("AAPVRBlocker: Not built with PVR API, caching block=false");
#endif

	return sShouldBlockCached;
}

void APPVRDeviceBlocker::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue){
	
	// get property container
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
	
	// get Prop_TrackingSystemName_String
	std::string trackingSystemName = vr::VRProperties()->GetStringProperty(container, vr::Prop_TrackingSystemName_String);
	
	
	
	// return and do nothing if not aapvr
	if(trackingSystemName != "aapvr"){
		return;
	}
	
	
	std::string model2 = vr::VRProperties()->GetStringProperty(container, vr::Prop_ModelNumber_String);
	if(!returnValue){
		DriverLog("AAPVRBlocker: Testing device: %s", model2.c_str());
	}
	
	// block aapvr headsets
	if(unObjectId == 0){
		DriverLog("AAPVRBlocker: Blocking AAPVR headset");
		returnValue = vr::EVRInitError::VRInitError_Init_HmdNotFound;
	}
	
	// block proxied lighthouse devices by checking for Prop_Firmware_ManualUpdateURL_String
	std::string firmwareManualUpdateURL = vr::VRProperties()->GetStringProperty(container, vr::Prop_Firmware_ManualUpdateURL_String);
	if(firmwareManualUpdateURL == "https://developer.valvesoftware.com/wiki/SteamVR/HowTo_Update_Firmware"){
		DriverLog("AAPVRBlocker: Blocking proxied lighthouse device");
		returnValue = vr::EVRInitError::VRInitError_Init_NotInitialized;
	}
	
	std::string model = vr::VRProperties()->GetStringProperty(container, vr::Prop_ModelNumber_String);
	if(!returnValue){
		DriverLog("AAPVRBlocker: AAPVR device activated: %s", model.c_str());
	}
}

#else // _WIN32

// Stub implementation for non-Windows platforms
void APPVRDeviceBlocker::PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue) {}

#endif // _WIN32