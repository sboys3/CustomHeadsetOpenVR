#include "HidModifier.h"
#include "DriverLog.h"
#include "../Config/ConfigLoader.h"

#include "../../../ThirdParty/minhook/include/MinHook.h"
#include "../../../ThirdParty/zlib/zlib.h"
#include "nlohmann/json.hpp"
#undef max
#undef min

#if HAS_PRIVATE
#include "HidModifierPrivate.cpp"
#endif


HidModifier hidModifier = HidModifier();


using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;


void HidModifier::InjectHooks(){
	if(hasHooked){
		return;
	}
	
	std::string lighthousePath = driverConfigLoader.info.steamvrResources + "../drivers/lighthouse/bin/win64/driver_lighthouse.dll";
	
	// resolve path
	char szPath[MAX_PATH];
	if(GetFullPathNameA(lighthousePath.c_str(), MAX_PATH, szPath, NULL) == 0){
		DriverLog("Failed to get full path for driver_lighthouse.dll");
		return;
	}
	lighthousePath = szPath;
	
	DriverLog("Loading lighthouse dll from: %s", lighthousePath.c_str());
	
	// load the lighthouse dll
	HMODULE hModule = LoadLibraryExA(lighthousePath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	
	// HMODULE hModule = GetModuleHandle(_T("driver_lighthouse.dll"));
	if(hModule){
		DriverLog("Got module handle for driver_lighthouse.dll");
	}else{
		DriverLog("Failed to get module handle for driver_lighthouse.dll");
		return;
	}
	
	
	auto err = MH_Initialize();
	if(err != MH_OK && err != MH_ERROR_ALREADY_INITIALIZED){
		DriverLog("Failed to initialize MinHook");
		return;
	}
	
	// Hook the hidapi function
	auto result = MH_CreateHook((void*)GetProcAddress(hModule, "hid_write"), &HidModifier::HidWriteHook, (void**)&origHidWrite);
	if(result == MH_OK){
		MH_EnableHook((void*)GetProcAddress(hModule, "hid_write"));
	}else{
		DriverLog("Failed to create hook for hid_write");
		return;
	}
	
	result = MH_CreateHook((void*)GetProcAddress(hModule, "hid_read_timeout"), &HidModifier::HidReadTimeoutHook, (void**)&origHidReadTimeout);
	if(result == MH_OK){
		MH_EnableHook((void*)GetProcAddress(hModule, "hid_read_timeout"));
	}else{
		DriverLog("Failed to create hook for hid_read_timeout");
		return;
	}
	
	result = MH_CreateHook((void*)GetProcAddress(hModule, "hid_get_feature_report"), &HidModifier::HidGetFeatureReportHook, (void**)&origHidGetFeatureReport);
	if(result == MH_OK){
		MH_EnableHook((void*)GetProcAddress(hModule, "hid_get_feature_report"));
	}else{
		DriverLog("Failed to create hook for hid_get_feature_report");
		return;
	}
	
	result = MH_CreateHook((void*)GetProcAddress(hModule, "hid_close"), &HidModifier::HidCloseHook, (void**)&origHidClose);
	if(result == MH_OK){
		MH_EnableHook((void*)GetProcAddress(hModule, "hid_close"));
	}else{
		DriverLog("Failed to create hook for hid_close");
		return;
	}
	
	origHidOpen = (hid_device* (*)(unsigned short vendor_id, unsigned short product_id, const wchar_t* serial_number))GetProcAddress(hModule, "hid_open");
	origHidOpenPath = (hid_device* (*)(const char* path))GetProcAddress(hModule, "hid_open_path");
	origHidError = (const wchar_t* (*)(hid_device* device))GetProcAddress(hModule, "hid_error");
	origHidEnumerate = (struct hid_device_info* (*)(unsigned short vendor_id, unsigned short product_id))GetProcAddress(hModule, "hid_enumerate");
	origHidFreeEnumeration = (void (*)(struct hid_device_info* devs))GetProcAddress(hModule, "hid_free_enumeration");
	
	if(!origHidOpen){
		DriverLog("Failed to get hid_open function");
		return;
	}
	if(!origHidOpenPath){
		DriverLog("Failed to get hid_open_path function");
		return;
	}
	if(!origHidError){
		DriverLog("Failed to get hid_error function");
		return;
	}
	if(!origHidEnumerate){
		DriverLog("Failed to get hid_enumerate function");
		return;
	}
	if(!origHidFreeEnumeration){
		DriverLog("Failed to get hid_free_enumeration function");
		return;
	}
	
	hasHooked = true;
	
	DriverLog("Hooks enabled");
}

void HidModifier::RunFrame(){
	if(!hasHooked){
		return;
	}
	// run all starters
	for(auto& pair : starters){
		pair.second->StartIfNeeded();
	}
}


void HidModifier::AddDevice(hid_device* device){
	if(deviceMap.find(device) != deviceMap.end()){
		return ;
	}
	// DriverLog("Adding HID device to HidWrite %i", device);
	HidDeviceInfo info;
	info.device = device;
	ReadLighthouseConfig(info);
	
	#ifdef PISTART
	PISTART
	#endif
	
	Sleep(10);
	deviceMap[device] = info;
}

void HidModifier::RemoveDevice(hid_device* device){
	if(deviceMap.find(device) == deviceMap.end()){
		return;
	}
	deviceMap[device].Delete();
	// DriverLog("Removing HID device from HidWrite %i", device);
	deviceMap.erase(device);
}

void mergeJson(ordered_json& base, const ordered_json& override) {
    for (auto it = override.begin(); it != override.end(); ++it) {
        if (base.contains(it.key()) && base[it.key()].is_object() && it.value().is_object()) {
            mergeJson(base[it.key()], it.value());
			// DriverLog("Merging json %s", it.key().c_str());
        } else {
            base[it.key()] = it.value();
			// DriverLog("Setting json %s", it.key().c_str());
        }
    }
}

std::string HidModifier::ReadLighthouseConfig(HidDeviceInfo &info){
	unsigned char buf[64];
	// start and get size
	buf[0] = 0x10;
	
	// try a few times to get the size of the config
	if(origHidGetFeatureReport(info.device, buf, 64) < 3){
		Sleep(2);
		if(origHidGetFeatureReport(info.device, buf, 64) < 3){
			Sleep(20);
			if(origHidGetFeatureReport(info.device, buf, 64) < 3){
				// DriverLog("Failed to read lighthouse config size");
				return "";
			}
		}
	}
	
	int size = buf[1] << 8 | buf[2];
	// DriverLog("Lighthouse size is %i", size);
	
	if(size > 20000){
		DriverLog("Lighthouse size is too large");
		return "";
	}
	
	unsigned char* configCompressed = new unsigned char[size + 64]{};
	int configCompressedLocation = 0;
	
	// read until we get all the data or reach the 0 read
	for(int i = 0; i < size / 16; i++){
		Sleep(1);
		// read data
		buf[0] = 0x11;
		if(origHidGetFeatureReport(info.device, buf, 64) < 2){
			continue;
		}
		int dataInLength = buf[1];
		if(dataInLength == 0 || configCompressedLocation + dataInLength > size + 60 || dataInLength > 62){
			break;
		}
		memcpy(configCompressed + configCompressedLocation, buf + 2, dataInLength);
		configCompressedLocation += dataInLength;
	}
	
	
	// test by writing the config to a file
	// FILE* file = fopen("lighthouse_config.bin", "wb");
	// if(file){
	// 	fwrite(configCompressed, 1, size, file);
	// 	fclose(file);
	// 	DriverLog("Wrote compressed lighthouse config from hid to file");
	// }else{
	// 	DriverLog("Failed to write lighthouse config to file");
	// }
	
	unsigned char* configDecompressed = new unsigned char[size * 10]{};
	uLongf decompressedSize = size * 10;
	int result = uncompress(configDecompressed, &decompressedSize, configCompressed, size);
	if(result == Z_OK){
		// DriverLog("Decompressed lighthouse config");
	}else{
		DriverLog("Failed to decompress lighthouse config");
		delete[] configCompressed;
		delete[] configDecompressed;
		return "";
	}
	delete[] configCompressed;
	
	// test by writing the config to a file
	// file = fopen("lighthouse_config_decompressed.json", "wb");
	// if(file){
	// 	fwrite(configDecompressed, 1, decompressedSize, file);
	// 	fclose(file);
	// 	DriverLog("Wrote decompressed lighthouse config from hid to file");
	// }else{
	// 	DriverLog("Failed to write decompressed lighthouse config to file");
	// }
	
	
	std::string configStr = std::string((char*)configDecompressed, decompressedSize);
	delete[] configDecompressed;
	bool doReplace = false;
	
	ordered_json data = ordered_json::parse(configStr, nullptr, true, true);
	
	if(data["model_number"].is_string()){
		info.deviceName = data["model_number"].get<std::string>();
	}
	
	if(data["device_class"].is_string() && data["device_class"].get<std::string>() == "hmd"){
		if(driverConfig.meganeX8K.edidVendorIdOverride != 0 && driverConfig.meganeX8K.enable){
			if(!data["direct_mode_edid_vid"].is_number() || data["direct_mode_edid_vid"].get<int>() != driverConfig.meganeX8K.edidVendorIdOverride){
				data["direct_mode_edid_vid"] = driverConfig.meganeX8K.edidVendorIdOverride;
				doReplace = true;
			}
		}
	}
	
	std::map<std::string, ordered_json> jsonOverrides = {};
	if(driverConfig.dreamAir.enable){
		jsonOverrides["Pimax Dream Air"] = {
			{"device", {
				{"eye_target_height_in_pixels", 3840},
				{"eye_target_width_in_pixels", 3552},
			}},
			{"direct_mode_edid_vid", 53826}, // PVR
			// {"device_class", "controller"}, 
		};
	}
	if(driverConfig.meganeX8K.enable){
		jsonOverrides["MeganeX superlight 8K"] = {
			{"device", {
				{"eye_target_height_in_pixels", 3840},
				{"eye_target_width_in_pixels", 3552},
			}},
		};
		jsonOverrides["MeganeX 8K Mark II"] = {
			{"device", {
				{"eye_target_height_in_pixels", 3840},
				{"eye_target_width_in_pixels", 3552},
			}},
		};
	}
	
	if(jsonOverrides.find(info.deviceName) != jsonOverrides.end()){
		mergeJson(data, jsonOverrides[info.deviceName]);
		doReplace = true;
	}
			
	
	if(doReplace){
		data["modified_by_custom_driver"] = true;
		std::string newConfigStr = data.dump(1);
		// DriverLogString(newConfigStr.c_str());
		
		unsigned char* configRecompressed = new unsigned char[size * 10]{};
		uLongf recompressedSize = size * 10;
		result = compress(configRecompressed, &recompressedSize, (unsigned char*)newConfigStr.c_str(), newConfigStr.length());
		if(result == Z_OK){
			DriverLog("Recompressed lighthouse config");
		}else{
			DriverLog("Failed to recompress lighthouse config");
		}
		// not sure why but these are always returned
		configRecompressed[recompressedSize] = 0xff;
		configRecompressed[recompressedSize + 1] = 0xff;
		info.newLighthouseConfig = configRecompressed;
		info.newLighthouseConfigLength = recompressedSize;
		// info.newLighthouseConfig = configCompressed;
		// info.newLighthouseConfigLength = size;
	}
	
	return configStr;
}



int HidModifier::HidWriteHook(hid_device* device, const unsigned char* data, size_t length){
	// DriverLog("HidWriteHook called");
	// Call the original function
	return origHidWrite(device, data, length);
}

int HidModifier::HidReadTimeoutHook(hid_device* device, unsigned char* data, size_t length, int milliseconds){
	
	// DriverLog("HidReadTimeoutHook called %i", length);
	// std::string str(data, data + length);
	// DriverLog("HidReadTimeoutHook called %i %i", data, length);
	// Call the original function
	return origHidReadTimeout(device, data, length, milliseconds);
}

int HidModifier::HidGetFeatureReportHook(hid_device* device, unsigned char* data, size_t length){
	hidModifier.AddDevice(device);
	
	HidDeviceInfo &info = hidModifier.deviceMap[device];
	if(info.newLighthouseConfig != nullptr){
		// output the new config
		if(data[0] == 0x10){
			// output new size
			data[1] = info.newLighthouseConfigLength >> 8;
			data[2] = info.newLighthouseConfigLength & 0xFF;
			info.newLighthouseConfigOffset = 0;
			// DriverLog("HidGetFeatureReportHook returning new config");
			return length;
		}else if(data[0] == 0x11){
			// output compressed chunk
			// the lighthouse driver supplies lengths of 65 but won't accept a buffer that size
			length = std::min(length, (size_t)64);
			int toCopy = std::min((int)(length - 2), (int)(info.newLighthouseConfigLength - info.newLighthouseConfigOffset + 2/*extra 0xff 0xff*/));
			memcpy(data + 2, info.newLighthouseConfig + info.newLighthouseConfigOffset, toCopy);
			info.newLighthouseConfigOffset += toCopy;
			data[1] = toCopy;
			// DriverLog("HidGetFeatureReportHook returning new config data %i", toCopy);
			return length;
		}
	}
	
	// Call the original function
	int result = origHidGetFeatureReport(device, data, length);
	if(result < 0){
		// DriverLog("HidGetFeatureReportHook failed %i", data[0]);
		return result;
	}
	length = result;
	// hex dump the data
	std::string dataStr;
	for(int i = 0; i < length; i++){
		char buf[3];
		sprintf(buf, "%02x", data[i]);
		dataStr += buf;
		if(i < length - 1){
			dataStr += ",";
		}
	}
	
	// DriverLog("HidGetFeatureReportHook called %i %s", result, dataStr.c_str());
	
	
	return result;
}

void HidModifier::HidCloseHook(hid_device* device){
	// DriverLog("HidCloseHook called");
	hidModifier.RemoveDevice(device);
	// Call the original function
	origHidClose(device);
}

int (*HidModifier::origHidWrite)(hid_device* device, const unsigned char* data, size_t length);
int (*HidModifier::origHidReadTimeout)(hid_device* device, unsigned char* data, size_t length, int milliseconds);
int (*HidModifier::origHidGetFeatureReport)(hid_device* device, unsigned char* data, size_t length);
void (*HidModifier::origHidClose)(hid_device* device);
HidModifier::hid_device* (*HidModifier::origHidOpen)(unsigned short vendor_id, unsigned short product_id, const wchar_t* serial_number);
HidModifier::hid_device* (*HidModifier::origHidOpenPath)(const char* path);
const wchar_t* (*HidModifier::origHidError)(hid_device* device);
HidModifier::hid_device_info* (*HidModifier::origHidEnumerate)(unsigned short vendor_id, unsigned short product_id);
void (*HidModifier::origHidFreeEnumeration)(struct hid_device_info* devs);
