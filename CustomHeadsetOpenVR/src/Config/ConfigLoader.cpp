#include "ConfigLoader.h"
#include <thread>
#include <fstream>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "../Driver/DriverLog.h"
#include "Windows.h"



using json = nlohmann::json;

std::string ConfigLoader::GetConfigFolder(){
	char* appdataPath = std::getenv("APPDATA");
	std::string configPath = appdataPath == nullptr ? "./" : (std::string(appdataPath) + "/CustomHeadset/");
	return configPath;
}

void ConfigLoader::ParseConfig(){
	// acquire driverConfig.configLock for the duration of this function
	std::lock_guard<std::mutex> lock(driverConfig.configLock);
	// get file at APPDATA/Roaming/CustomHeadset/settings.json
	std::string configPath = GetConfigFolder() + "settings.json";
	std::ifstream configFile(configPath);
	if(!configFile.is_open()){
		if(!hasLoggedConfigFileNotFound){
			hasLoggedConfigFileNotFound = true;
			DriverLog("Config file not found at %s, using default settings.", configPath.c_str());
		}
		return;
	}
	DriverLog("Loading config file from %s", configPath.c_str());
	try{
		// parse with support for comments
		json data = json::parse(configFile, nullptr, true, true);
		if(data["meganeX8K"].is_object()){
			json meganeX8KData = data["meganeX8K"];
			if(meganeX8KData["enable"].is_boolean()){
				driverConfig.meganeX8K.enable = meganeX8KData["enable"].get<bool>();
			}
			if(meganeX8KData["ipd"].is_number()){
				driverConfig.meganeX8K.ipd = meganeX8KData["ipd"].get<double>();
			}
			if(meganeX8KData["blackLevel"].is_number()){
				driverConfig.meganeX8K.blackLevel = meganeX8KData["blackLevel"].get<double>();
			}
		}
	}catch(const std::exception& e){
		DriverLog("Failed to parse config file: %s", e.what());
		return;
	}
	// mark as updated
	driverConfig.hasBeenUpdated = true;
}

void ConfigLoader::WatcherThread(){
	// watch for changes in the config file directory
	std::string configPath = GetConfigFolder();
	HANDLE hDir = CreateFileA(configPath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(hDir == INVALID_HANDLE_VALUE){
		DriverLog("Failed to open config directory for watching: %d", GetLastError());
		return;
	}
	while(started){
		DWORD bytesReturned;
		char buffer[1024];
		FILE_NOTIFY_INFORMATION* pNotify;
		BOOL success = ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL);
		if(!success){
			DriverLog("Failed to read directory changes: %d", GetLastError());
			break;
		}
		pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
		do{
			std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t));
			if(fileName == L"settings.json" && (pNotify->Action == FILE_ACTION_MODIFIED || pNotify->Action == FILE_ACTION_ADDED || pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME)){
				DriverLog("Config file changed, reloading...");
				ParseConfig();
			}
			pNotify = (FILE_NOTIFY_INFORMATION*)((char*)pNotify + pNotify->NextEntryOffset);
		}while(pNotify->NextEntryOffset != 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

void ConfigLoader::Start(){
	if(started){
		return;
	}
	started = true;
	// load config for the first time
	ParseConfig();
	// create directory and start watcher thread
	std::filesystem::create_directories(GetConfigFolder());
	std::thread watcher(&ConfigLoader::WatcherThread, this);
	// Detach the thread to run independently
	watcher.detach();
}
