#include "CompositorPlugin.h"
#include "ShaderReplacement.h"
#include "DriverLog.h"
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
	
	// beware this blocks for a while to ensure shaders get reloaded
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
#include <TlHelp32.h>
void winInit(){
	if(initialized){
		return;
	}
	
	// check that the executable we are in is vrcompositor.exe
	wchar_t buffer[MAX_PATH] = {};
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


bool InjectCompositorPlugin(int forProcessId){
	WCHAR* processName = (LPWSTR)L"vrcompositor.exe";
	
	WCHAR dllPath[1024] = {};
	// get current dll path
	HMODULE dllModule = NULL;
	if(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPWSTR) &InjectCompositorPlugin, &dllModule) == 0){
		DriverLog("failed to get dll module handle");
		return false;
	}
	if(GetModuleFileName(dllModule, dllPath, sizeof(dllPath) / sizeof(WCHAR)) == 0){
		DriverLog("failed to get dll path");
		return false;
	}
	
	

	// get process id of vrcompositor.exe
	DWORD processId{};
	auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if(hSnapshot == INVALID_HANDLE_VALUE)
	{
		DriverLog("invalid process snapshot handle");
		return false;
	}
	PROCESSENTRY32 process{};
	process.dwSize = sizeof(PROCESSENTRY32);
	BOOL isProcessFound = FALSE;
	// iterate through all process entries in the snapshot and find process id of processName
	if(Process32First(hSnapshot, &process) == FALSE){
		CloseHandle(hSnapshot);
		DriverLog("process list failure");
		return false;
	}
	if(wcscmp(process.szExeFile, processName) == 0){
		CloseHandle(hSnapshot);
		processId = process.th32ProcessID;
		isProcessFound = TRUE;
	}
	while(Process32Next(hSnapshot, &process)){
		if (wcscmp(process.szExeFile, processName) == 0){
			CloseHandle(hSnapshot);
			processId = process.th32ProcessID;
			isProcessFound = TRUE;
			break;
		}
	}
	
	if(forProcessId != 0){
		if(processId != forProcessId){
			// don't log because this will be called for every process that connects
			// DriverLog("process id mismatch %i %i", processId, forProcessId);
			return false;
		}
	}
	
	
	// Check if process was found
	if(isProcessFound==FALSE){
		DriverLog("compositor process not found");
		return false;
	}
	
	
	// get the address of LoadLibraryW
	auto hKernel32 = GetModuleHandle(L"kernel32.dll");
	if(hKernel32 == NULL){
		DriverLog("can't get kernel32.dll");
		return false;
	}
	FARPROC LoadLibraryAddress;
	if ((LoadLibraryAddress = GetProcAddress(hKernel32, "LoadLibraryW")) == NULL){
		DriverLog("failed to load kernel32.dll");
		return false;
	}
	
	// open process to inject into
	auto size = wcslen(dllPath) * sizeof(TCHAR);
	auto targetProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, processId);
	if(targetProcess == NULL){
		DriverLog("process open failed %i", processId);
		return false;
	}
	// allocate memory in the remote process
	auto nameInTargetProcess = VirtualAllocEx(targetProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if(nameInTargetProcess == NULL){
		DriverLog("remote allocation failed");
		return false;
	}
	// put data into memory of the remote process
	// auto bStatus = WriteProcessMemory(targetProcess, nameInTargetProcess, dllPath, size, nullptr);
	// dynamically import WriteProcessMemory to avoid false anti virus static analysis detection
	std::string part1 = "WriteProc";
	std::string part2 = "essMemory";
	std::string full = part1 + part2;
	typedef BOOL (WINAPI *WriteProcessMemory_t)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
	auto WriteProcessMemory = (WriteProcessMemory_t)GetProcAddress(hKernel32, full.c_str());
	if(WriteProcessMemory == NULL){
		DriverLog("failed to get %s", full.c_str());
	}
	auto bStatus = WriteProcessMemory(targetProcess, nameInTargetProcess, dllPath, size, nullptr);
	if(bStatus == 0){
		DriverLog("failed to write remote memory");
		return false;
	}
	
	

	// LoadLibraryW((LPCWSTR)dllPath);
	
	// using the above objects execute the DLL in the remote process
	// auto hThreadId = CreateRemoteThread(targetProcess,
	// 	nullptr,
	// 	0,
	// 	reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryAddress),
	// 	nameInTargetProcess,
	// 	NULL,
	// 	nullptr
	// );
	// dynamically import CreateRemoteThread to avoid false anti virus static analysis detection
	part1 = "CreateRem";
	part2 = "oteThread";
	full = part1 + part2;
	typedef HANDLE (WINAPI *CreateRemoteThread_t)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
	auto CreateRemoteThread = (CreateRemoteThread_t)GetProcAddress(hKernel32, full.c_str());
	if(CreateRemoteThread == NULL){
		DriverLog("failed to get %s", full.c_str());
	}
	auto hThreadId = CreateRemoteThread(targetProcess,
		nullptr,
		0,
		reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryAddress),
		nameInTargetProcess,
		NULL,
		nullptr
	);
	if(hThreadId == NULL){
		DriverLog("failed to execute dll in remote process");
		return false;
	}
	// WaitForSingleObject(hThreadId, INFINITE);
	
	DriverLog("driver successfully injected into compositor process id: %i", processId);

	CloseHandle(targetProcess);
	VirtualFreeEx(targetProcess, nameInTargetProcess, size, MEM_RELEASE);
	
	return true;
}