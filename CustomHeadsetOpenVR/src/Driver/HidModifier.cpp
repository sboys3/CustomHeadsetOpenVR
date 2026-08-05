#include "HidModifier.h"
#include "DriverLog.h"
#include "../Config/ConfigLoader.h"
#include "../Helpers/PimaxCommon.h"
#include "../Helpers/MiscHelper.h"

#ifdef __linux__
#include <unistd.h>
#endif
#include "../../../ThirdParty/LightHook/Source/LightHook.h"
#include "../../../ThirdParty/zlib/zlib.h"
#include "nlohmann/json.hpp"
#undef max
#undef min

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>  // realpath
#endif

#if HAS_PRIVATE
#include "HidModifierPrivate.h"
#endif


HidModifier hidModifier = HidModifier();


using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;


void HidModifier::InjectHooks(){
	if(hasHooked){
		return;
	}
	
#ifdef _WIN32
	std::string lighthousePath = driverConfigLoader.info.steamvrResources + "../drivers/lighthouse/bin/win64/driver_lighthouse.dll";
#else
	// hid functions are not actually exported on Linux
	return;
	std::string lighthousePath = driverConfigLoader.info.steamvrResources + "../drivers/lighthouse/bin/linux64/driver_lighthouse.so";
#endif
	
	
#ifdef _WIN32
	// resolve path
	char szPath[MAX_PATH];
	if(GetFullPathNameA(lighthousePath.c_str(), MAX_PATH, szPath, NULL) == 0){
		DriverLog("Failed to get full path for driver_lighthouse.dll");
		return;
	}
	lighthousePath = szPath;
#endif
	
	DriverLog("Loading lighthouse dll from: %s", lighthousePath.c_str());
	
	// load the lighthouse driver
	void* hModule = LibOpen(lighthousePath);
	
	if(hModule){
		DriverLog("Got module handle for driver_lighthouse.dll");
	}else{
		DriverLog("Failed to get module handle for driver_lighthouse.dll");
		return;
	}
	
	// Hook the hidapi functions using LightHook
	void* addrHidWrite = LibAddress(hModule, "hid_write");
	if(!addrHidWrite){ DriverLog("Failed to resolve hid_write"); return; }
	hookHidWrite = new HookInformation(CreateHook(addrHidWrite, (void*)&HidModifier::HidWriteHook));
	if(!EnableHook((HookInformation*)hookHidWrite)){
		DriverLog("Failed to create hook for hid_write");
		return;
	}
	origHidWrite = (int (*)(hid_device*, const unsigned char*, size_t))((HookInformation*)hookHidWrite)->Trampoline;
	
	void* addrHidReadTimeout = LibAddress(hModule, "hid_read_timeout");
	if(!addrHidReadTimeout){ DriverLog("Failed to resolve hid_read_timeout"); return; }
	hookHidReadTimeout = new HookInformation(CreateHook(addrHidReadTimeout, (void*)&HidModifier::HidReadTimeoutHook));
	if(!EnableHook((HookInformation*)hookHidReadTimeout)){
		DriverLog("Failed to create hook for hid_read_timeout");
		return;
	}
	origHidReadTimeout = (int (*)(hid_device*, unsigned char*, size_t, int))((HookInformation*)hookHidReadTimeout)->Trampoline;
	
	void* addrHidGetFeatureReport = LibAddress(hModule, "hid_get_feature_report");
	if(!addrHidGetFeatureReport){ DriverLog("Failed to resolve hid_get_feature_report"); return; }
	hookHidGetFeatureReport = new HookInformation(CreateHook(addrHidGetFeatureReport, (void*)&HidModifier::HidGetFeatureReportHook));
	if(!EnableHook((HookInformation*)hookHidGetFeatureReport)){
		DriverLog("Failed to create hook for hid_get_feature_report");
		return;
	}
	origHidGetFeatureReport = (int (*)(hid_device*, unsigned char*, size_t))((HookInformation*)hookHidGetFeatureReport)->Trampoline;
	
	void* addrHidClose = LibAddress(hModule, "hid_close");
	if(!addrHidClose){ DriverLog("Failed to resolve hid_close"); return; }
	hookHidClose = new HookInformation(CreateHook(addrHidClose, (void*)&HidModifier::HidCloseHook));
	if(!EnableHook((HookInformation*)hookHidClose)){
		DriverLog("Failed to create hook for hid_close");
		return;
	}
	origHidClose = (void (*)(hid_device*))((HookInformation*)hookHidClose)->Trampoline;
	
	origHidOpen = (hid_device* (*)(unsigned short vendor_id, unsigned short product_id, const wchar_t* serial_number))LibAddress(hModule, "hid_open");
	origHidOpenPath = (hid_device* (*)(const char* path))LibAddress(hModule, "hid_open_path");
	origHidError = (const wchar_t* (*)(hid_device* device))LibAddress(hModule, "hid_error");
	origHidEnumerate = (struct hid_device_info* (*)(unsigned short vendor_id, unsigned short product_id))LibAddress(hModule, "hid_enumerate");
	origHidFreeEnumeration = (void (*)(struct hid_device_info* devs))LibAddress(hModule, "hid_free_enumeration");
	
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
	
	#ifdef PIPRESTART
	PIPRESTART
	#endif
}

void HidModifier::RemoveHooks(){
	if(!hasHooked){
		return;
	}
	
	DriverLog("Removing lighthouse hidapi hooks");
	
	if(hookHidWrite){
		DisableHook((HookInformation*)hookHidWrite);
		delete (HookInformation*)hookHidWrite;
		hookHidWrite = nullptr;
	}
	if(hookHidReadTimeout){
		DisableHook((HookInformation*)hookHidReadTimeout);
		delete (HookInformation*)hookHidReadTimeout;
		hookHidReadTimeout = nullptr;
	}
	if(hookHidGetFeatureReport){
		DisableHook((HookInformation*)hookHidGetFeatureReport);
		delete (HookInformation*)hookHidGetFeatureReport;
		hookHidGetFeatureReport = nullptr;
	}
	if(hookHidClose){
		DisableHook((HookInformation*)hookHidClose);
		delete (HookInformation*)hookHidClose;
		hookHidClose = nullptr;
	}
	
	// Clean up device map
	for(auto& pair : deviceMap){
		pair.second.Delete();
	}
	deviceMap.clear();
	originalDeviceClasses.clear();
	starters.clear();
	
	hasHooked = false;
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
	
	SleepMs(10);
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
		SleepMs(2);
		if(origHidGetFeatureReport(info.device, buf, 64) < 3){
			SleepMs(20);
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
		SleepMs(1);
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
	
	ordered_json data = ordered_json::parse(configStr, nullptr, false, true);
	
	// Apply user-defined lighthouse overrides before reading any fields
	for (auto& override : driverConfig.lighthouseOverrides) {
		if (!override.enable) continue;
		// match against the pre-override values so we can identify the device first
		std::string currentName = data["model_number"].is_string() ? data["model_number"].get<std::string>() : "";
		std::string currentSerial = data["device_serial_number"].is_string() ? data["device_serial_number"].get<std::string>() : "";
		bool matches = false;
		if (!override.targetName.empty() && currentName == override.targetName) matches = true;
		if (!override.targetSerial.empty() && currentSerial == override.targetSerial) matches = true;
		if (matches && override.overrideData != nullptr) {
			ordered_json* overrideJson = static_cast<ordered_json*>(override.overrideData);
			mergeJson(data, *overrideJson);
			doReplace = true;
			DriverLog("HidModifier - Applied lighthouse override for %s/%s", override.targetName.c_str(), override.targetSerial.c_str());
		}
	}
	
	if(data["model_number"].is_string()){
		info.lighthouseDeviceName = data["model_number"].get<std::string>();
	}
	if(data["manufacturer"].is_string()){
		info.lighthouseDeviceManufacturer = data["manufacturer"].get<std::string>();
	}
	if(data["device_class"].is_string()){
		info.lighthouseDeviceClass = data["device_class"].get<std::string>();
	}
	if(data["device_serial_number"].is_string()){
		info.lighthouseDeviceSerial = data["device_serial_number"].get<std::string>();
	}
	
	DriverLog("HidModifier - Lighthouse device name: %s, manufacturer: %s", info.lighthouseDeviceName.c_str(), info.lighthouseDeviceManufacturer.c_str());
	
	std::map<std::string, ordered_json> jsonOverrides = {};
	if(driverConfig.meganeX8K.enable){
		int vendorId = driverConfig.meganeX8K.edidVendorIdOverride ? driverConfig.meganeX8K.edidVendorIdOverride : driverConfig.meganeX8K.edidVendorId;
		jsonOverrides["MeganeX superlight 8K"] = {
			{"device", {
				{"eye_target_width_in_pixels", driverConfig.meganeX8K.resolutionY},
				{"eye_target_height_in_pixels", driverConfig.meganeX8K.resolutionX},
			}},
			{"direct_mode_edid_vid", vendorId},
		};
		jsonOverrides["MeganeX 8K Mark II"] = {
			{"device", {
				{"eye_target_width_in_pixels", driverConfig.meganeX8K.resolutionY},
				{"eye_target_height_in_pixels", driverConfig.meganeX8K.resolutionX},
			}},
			{"direct_mode_edid_vid", vendorId},
		};
	}
	if(PimaxCommon::IsPimaxLighthouseDevice(info.lighthouseDeviceName, info.lighthouseDeviceManufacturer)){
		// attempt to directly connect to the Pimax headset that this lighthouse device belongs to
		PimaxCommon::TryDirectConnection();
	}
	if(PimaxCommon::IsLighthouseHeadsetConnected()){
		auto pimaxInfo = PimaxCommon::GetInfo();
		Config::BaseHeadsetConfig& pimaxConfig = PimaxCommon::GetHeadsetConfig();
		int vendorId = pimaxConfig.edidVendorIdOverride ? pimaxConfig.edidVendorIdOverride : pimaxConfig.edidVendorId;
		ordered_json pimaxOverride = {
			{"device", {
				{"eye_target_width_in_pixels", pimaxConfig.resolutionX},
				{"eye_target_height_in_pixels", pimaxConfig.resolutionY},
			}},
			{"direct_mode_edid_vid", vendorId}, // Probably PVR
		};
		switch(pimaxInfo.headsetType){
			case Config::HeadsetType::DreamAir:
				jsonOverrides["Pimax Dream Air"] = pimaxOverride;
				break;
			case Config::HeadsetType::DreamAirSE:
				jsonOverrides["Pimax Dream Air SE"] = pimaxOverride;
				jsonOverrides["Pimax Dream Air"] = pimaxOverride;
				break;
			case Config::HeadsetType::CrystalSuper50PPD:
			case Config::HeadsetType::CrystalSuper57PPD:
			case Config::HeadsetType::CrystalSuperUltrawide:
			case Config::HeadsetType::CrystalSuperMicroOLED:
				jsonOverrides["Pimax Crystal Super"] = pimaxOverride;
				// also allow janky crystal faceplates on the super so don't break
			default:
				jsonOverrides["REF-HMD"] = pimaxOverride;
		}
	}
	
	if(jsonOverrides.find(info.lighthouseDeviceName) != jsonOverrides.end()){
		mergeJson(data, jsonOverrides[info.lighthouseDeviceName]);
		doReplace = true;
	}
	
	
	if(info.lighthouseDeviceClass == "hmd"){
		if(driverConfig.meganeX8K.enable && driverConfig.meganeX8K.forceEnable){
			int vendorId = driverConfig.meganeX8K.edidVendorIdOverride ? driverConfig.meganeX8K.edidVendorIdOverride : driverConfig.meganeX8K.edidVendorId;
			if(!data["direct_mode_edid_vid"].is_number() || data["direct_mode_edid_vid"].get<int>() != vendorId){
				data["direct_mode_edid_vid"] = vendorId;
				doReplace = true;
			}
		}
		if(driverConfig.dreamAir.enable && driverConfig.dreamAir.forceEnable){
			int vendorId = driverConfig.dreamAir.edidVendorIdOverride ? driverConfig.dreamAir.edidVendorIdOverride : driverConfig.dreamAir.edidVendorId;
			if(!data["direct_mode_edid_vid"].is_number() || data["direct_mode_edid_vid"].get<int>() != vendorId){
				data["direct_mode_edid_vid"] = vendorId;
				doReplace = true;
			}
		}
	}
	
	if(driverConfig.generalHeadset.lighthouseCalibrationDeviceOverride != "" && (info.lighthouseDeviceClass == "hmd" || info.lighthouseDeviceClass == "controller")){
		if(info.lighthouseDeviceName == driverConfig.generalHeadset.lighthouseCalibrationDeviceOverride || info.lighthouseDeviceSerial == driverConfig.generalHeadset.lighthouseCalibrationDeviceOverride){
			// if(info.lighthouseDeviceClass != "hmd"){
			// 	originalDeviceClasses[info.lighthouseDeviceSerial] = vr::TrackedDeviceClass_Controller;
			// 	DriverLog("Overriding device class for %s to hmd", info.lighthouseDeviceSerial.c_str());
			// 	data["device_class"] = "hmd";
			// 	doReplace = true;
			// }
			if(info.lighthouseDeviceClass == "hmd"){
				// originalDeviceClasses[info.lighthouseDeviceSerial] = vr::TrackedDeviceClass_HMD;
				originalDeviceClasses[info.lighthouseDeviceSerial] = vr::TrackedDeviceClass_GenericTracker;
				DriverLog("Overriding device class for %s to controller", info.lighthouseDeviceSerial.c_str());
				data["device_class"] = "controller";
				doReplace = true;
			}
		}else{
			if(info.lighthouseDeviceClass == "hmd"){
				originalDeviceClasses[info.lighthouseDeviceSerial] = vr::TrackedDeviceClass_HMD;
				DriverLog("Overriding device class for %s to controller", info.lighthouseDeviceSerial.c_str());
				data["device_class"] = "controller";
				doReplace = true;
			}
		}
	}
	
	if(doReplace){
		data["modified_by_custom_driver"] = true;
		std::string newConfigStr = data.dump(1);
		// DriverLogString(newConfigStr.c_str());
		
		unsigned char* configRecompressed = new unsigned char[size * 10]{};
		uLongf recompressedSize = size * 10 - 2;
		result = compress(configRecompressed, &recompressedSize, (unsigned char*)newConfigStr.c_str(), newConfigStr.length());
		if(result == Z_OK){
			DriverLog("Recompressed lighthouse config");
		}else{
			DriverLog("Failed to recompress lighthouse config");
			delete[] configRecompressed;
			return configStr;
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
	
	#ifdef HAS_PRIVATE
	if(driverConfig.onlyHandlePrivateFunctionality){
		return origHidGetFeatureReport(device, data, length);
	}
	#endif
	
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
