#include "PimaxCommon.h"

#ifdef PVR_EXISTS
#include <PVR_Math.h>
#endif
#include <hidapi/hidapi.h>
#include <chrono>
#include <optional>
#include <vector>
#include <shared_mutex>
#include <thread>

#include "../Driver/DriverLog.h"
#include "../Driver/DeviceProvider.h"
#include "EyeTrackingOutput.h"
#include "MiscHelper.h"

// For "hmd_buttons" config. Not in PVR SDK.
enum class HmdButton : int {
	Button_None = 0x0000,
	Button_Power = 0x0001,
	Button_VolumeUp = 0x0002,
	Button_VolumeDown = 0x0004,
	Button_DoubleTap = 0x0008,
};

#ifdef PVR_EXISTS
static pvrEnvHandle s_pvr = {};
static pvrSessionHandle s_pvrSession = {};
#endif
std::shared_mutex PimaxCommon::pvrLock;
static PimaxInfo s_info = {};

int directButtonState = 0;
int pvrButtonState = 0;

bool HeadsetHasEyeTracking(Config::HeadsetType type){
	// Crystal OG, Crystal Super (all variants), Dream Air SE, Dream Air.
	return type == Config::HeadsetType::CrystalOG ||
		type == Config::HeadsetType::CrystalSuper50PPD ||
		type == Config::HeadsetType::CrystalSuper57PPD ||
		type == Config::HeadsetType::CrystalSuperUltrawide ||
		type == Config::HeadsetType::CrystalSuperMicroOLED ||
		type == Config::HeadsetType::DreamAirSE ||
		type == Config::HeadsetType::DreamAir;
}

Config::HeadsetType TypeFromProduct(int productId, std::string_view productName, std::string_view serialNumber){
	if (productId == 0x0044 || productId == 0x0043 || productName == "Pimax Dream Air") { // Dream Air (0x0043 is USB only)
		return Config::HeadsetType::DreamAir;
	}
	if (productId == 0x0042 || productName == "Pimax Dream Air SE") { // Dream Air SE
		return Config::HeadsetType::DreamAirSE;
	}
	if (productId == 0x0040 || productName == "Pimax Crystal Super") { // Crystal Super (50PPD, 57PPD, Ultrawide, MicroOLED share this ProductId)
		return Config::HeadsetType::CrystalSuper50PPD;
	}
	if (productId == 0x0018) { // Crystal Light
		return Config::HeadsetType::CrystalLight;
	}
	if (productId == 0x0012) { // Crystal OG
		return Config::HeadsetType::CrystalOG;
	}
	if (productId == 0x0101) { // Pimax 5K and 8K series (share the same ProductId)
		if (productName.find("5K") != std::string::npos) {
			return Config::HeadsetType::Pimax5KPlus;
		} else if (productName.find("Artisan") != std::string::npos) {
			return Config::HeadsetType::PimaxArtisan;
		} else {
			return Config::HeadsetType::Pimax8KX;
		}
	}
	return Config::HeadsetType::None;
}

class PimaxDirectUSB {
public:
	PimaxInfo info = {};

	bool running = false;
	bool firstRun = true;
	bool isP3Headset = false;
	double lastRun = 0;
	std::string log = "";
	hid_device* device = nullptr;
	hid_device* device2 = nullptr;
	std::mutex hidLock = {};
	
	std::optional<std::vector<uint8_t>> SendCommandWithResult(const std::vector<uint8_t>& cmd, double timeout = 0.5){
		std::vector<uint8_t> report(65, 0);
		for(size_t i = 0; i < cmd.size(); i++){
			report[i + 1] = cmd[i];
		}
		int ret = 0;
		{
			std::lock_guard<std::mutex> lock(hidLock);
			if(!device){
				return std::nullopt;
			}
			ret = hid_write(device, report.data(), 65);
		}
		if(ret != 65){
			return std::nullopt;
		}
		auto startTime = std::chrono::steady_clock::now();
		while(true){
			std::vector<uint8_t> data(64);
			{
				std::lock_guard<std::mutex> lock(hidLock);
				if(!device){
					return std::nullopt;
				}
				ret = hid_read(device, data.data(), 64);
			}
			if(ret == 64){
				if(data[0] == 0x4C && data[2] == report[3] && data[3] == report[4]){
					return data;
				}
			}
			auto now = std::chrono::steady_clock::now();
			double elapsed = std::chrono::duration<double>(now - startTime).count();
			if(elapsed >= timeout){
				break;
			}
			SleepMs(5);
		}
		return std::nullopt;
	}

	std::string GetSerial(uint8_t id){
		std::vector<uint8_t> cmd = { 0x58, 0x04, 0x16, id, 0x20 };
		auto data = SendCommandWithResult(cmd);
		if(!data){
			return "";
		}
		uint8_t serialLength = (*data)[4];
		std::string serialData(static_cast<int>(serialLength), 0);
		for(uint8_t i = 0; i < serialLength; i++){
			serialData[i] = static_cast<char>((*data)[5 + i]);
		}
		return serialData;
	}
	
	Config::HeadsetType GetSuperModuleType(){
		std::vector<uint8_t> cmd = { 0x58, 0x04, 0x3A, 0x05, 0x01 };
		auto data = SendCommandWithResult(cmd);
		if(!data){
			return Config::HeadsetType::None;
		}
		uint8_t moduleTypeId = (*data)[5];
		DriverLog("Super module type ID: 0x%02X\n", moduleTypeId);
		switch(moduleTypeId){
			case 0x05:
				return Config::HeadsetType::CrystalSuperMicroOLED;
			case 0x11:
				return Config::HeadsetType::CrystalSuper57PPD;
			case 0x22:
				return Config::HeadsetType::CrystalSuper50PPD;
			case 0x44:
				return Config::HeadsetType::CrystalSuperUltrawide;
			default:
				return Config::HeadsetType::None;
		}
	} 
	
	bool SetIpd(float ipdMm){
		uint16_t ipdValue = static_cast<uint16_t>(ipdMm * 100);
		uint8_t ipdLow = ipdValue & 0xFF;
		uint8_t ipdHigh = (ipdValue >> 8) & 0xFF;
		std::vector<uint8_t> cmd = { 0x58, 0x06, 0x5C, 0x05, 0x01, ipdLow, ipdHigh };
		auto data = SendCommandWithResult(cmd);
		if(!data){
			return false;
		}
		return (*data)[4] == 1;
	}

	float GetIpd(double timeout = 0.01){
		auto startTime = std::chrono::steady_clock::now();
		while(true){
			int ret;
			std::vector<uint8_t> data(64);
			{
				std::lock_guard<std::mutex> lock(hidLock);
				if(!device2){
					return 0;
				}
				ret = hid_read(device2, data.data(), 64);
			}
			if(ret == 64){
				if(data[0] == 0x53 && data[2] == 0x08 && data[3] == 0x00){
					uint16_t ipdValue = data[45] | (data[46] << 8);
					return ipdValue / 100.0f;
				}
			}
			auto now = std::chrono::steady_clock::now();
			double elapsed = std::chrono::duration<double>(now - startTime).count();
			if(elapsed >= timeout){
				break;
			}
			SleepMs(1);
		}
		return 0;
	}
	
	
	
	void InfoThread(){
		while(running && device){
			int ret;
			std::vector<uint8_t> infoData(64);
			{
				std::lock_guard<std::mutex> lock(hidLock);
				if(device2){
					ret = hid_read(device2, infoData.data(), 64);
				}
			}
			if(ret == 64 && isP3Headset){
				if(infoData[0] == 0x53 && infoData[2] == 0x08 && infoData[3] == 0x00){
					uint16_t ipdValue = infoData[45] | (infoData[46] << 8);
					s_info.ipd = ipdValue / 100.0f;
					directButtonState = infoData[49];
				}
				continue;
			}
			if(ret == -1){
				// stop running on error
				running = false;
			}
			SleepMs(10);
		}
		running = false;
	}
	bool Start(){
		// don't leave this as it can crash if log is used in the loop
		// if(log != ""){
		// 	DriverLog("PError %s", log.c_str());
		// 	log = "";
		// }
		if(running){
			return false;
		}
		double now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
		if(now - lastRun < 5){
			return false;
		}
		lastRun = now;
		{
			// lock the mutex
			std::lock_guard<std::mutex> lock(hidLock);
			Close();
		}
		hid_device_info* devices = hid_enumerate(0x34A4, 0);
		if(!devices){
			DriverLog("No Pimax devices found");
			return false;
		}
		auto devicesIter = devices;
		int productId = 0;
		std::string deviceName = "";
		std::string serialNumber = "";
		std::string path = "";
		std::string path2 = "";
		while(devicesIter){
			char name[256] = {};
			char serial[256] = {};
			int i = 0;
			if(devicesIter->product_string){
				while(i < 255 && devicesIter->product_string[i]){
					// screw wide wchar_t
					name[i] = devicesIter->product_string[i];
					i++;
				}
			}
			i = 0;
			if(devicesIter->serial_number){
				while(i < 255 && devicesIter->serial_number[i]){
					serial[i] = devicesIter->serial_number[i];
					i++;
				}
			}
			if(serial[0] == 'P'){
				std::string maskedSerial = serial;
				// Mask last four digits with X's
				if(maskedSerial.length() > 4){
					maskedSerial = maskedSerial.substr(0, maskedSerial.length() - 4) + "XXXX";
				}
				DriverLog("Found Pimax device %i, %s, %s, %s", devicesIter->interface_number, name, maskedSerial.c_str(), devicesIter->path);
				if(devicesIter->interface_number == 0){
					path = devicesIter->path;
					deviceName = name;
					serialNumber = serial;
					productId = devices->product_id;
					// break;
				}
				if(devicesIter->interface_number == 2){
					path2 = devicesIter->path;
					// break;
				}
			}
			devicesIter = devicesIter->next;
		}
		hid_free_enumeration(devices);
		Config::HeadsetType headsetType = TypeFromProduct(productId, deviceName, serialNumber);
		if(headsetType == Config::HeadsetType::None){
			DriverLog("Unknown Pimax device %i, %s, %s", productId, deviceName.c_str(), serialNumber.c_str());
			return false;
		}
		isP3Headset = headsetType == Config::HeadsetType::DreamAir || headsetType == Config::HeadsetType::DreamAirSE || headsetType == Config::HeadsetType::CrystalSuper50PPD || headsetType == Config::HeadsetType::CrystalLight || headsetType == Config::HeadsetType::CrystalOG;
		if(!isP3Headset){
			// Return false as non-p3 headsets rely on the Pimax API for proper distortion and eye rotations, so for now don't allow them to direct connect.
			return false;
			// other headsets are not supported yet for USB functions, but they are still identified
			// s_info.headsetType = headsetType;
			// s_info.directConnected = true;
			// running = true;
			// return true;
		}
		{
			// lock the mutex
			std::lock_guard<std::mutex> lock(hidLock);
			device = hid_open_path(path.c_str());
			if(!device){
				// get last error
				const wchar_t* error = hid_error(NULL);
				char errorStr[256] = {};
				for(int i = 0; i < 255 && error[i]; i++){
					errorStr[i] = error[i];
				}
				DriverLog("Error: %s", errorStr);
				DriverLog("Failed to open Pimax device");
				return false;
			}
			hid_set_nonblocking(device, 1);
			device2 = hid_open_path(path2.c_str());
			if(!device2){
				// Only needed for statistics and preventing USB reconnects on Linux.
				DriverLog("Failed to open Pimax device 2");
			}else{
				hid_set_nonblocking(device2, 1);
			}
		}
		if(headsetType == Config::HeadsetType::CrystalSuper50PPD){
			headsetType = GetSuperModuleType();
			if(headsetType == Config::HeadsetType::None){
				Close();
				return false;
			}
			// Also log the module serial number, but with the last four digits replaced with X's.
			std::string serial = GetSerial(0x05);
			if(serial.length() > 4){
				std::string maskedSerial = serial.substr(0, serial.size() - 4) + "XXXX";
				DriverLog("Super module serial: %s", maskedSerial.c_str());
			}
			int edidVidOverride = 0;
			// Module serial numbers starting in P407V0D have the PVR edid vendor
			if(serial.find("P407V0D") == 0){
				edidVidOverride = 53826;
			}
			Config::BaseHeadsetConfig* config = driverConfig.ConfigFromHeadsetType(s_info.headsetType);
			if(config && edidVidOverride && config->edidVendorId != edidVidOverride){
				DriverLog("EDID vendor ID mismatch: expected %i, guessing %i\n", config->edidVendorId, edidVidOverride);
				config->edidVendorId = edidVidOverride;
			}
		}
		s_info.headsetType = headsetType;
		s_info.hasEyeTracking = HeadsetHasEyeTracking(headsetType);
		running = true;
		std::thread t(&PimaxDirectUSB::InfoThread, this);
		t.detach();
		DriverLog("Opened Pimax device");
		if(firstRun){
			firstRun = false;
			// wait a bit for info
			SleepMs(2000);
			// SleepMs(10000);
		}
		return true;
	}
	// You must acquire the lock before calling this function
	void Close(){
		if(device){
			DriverLog("Closing Pimax device %s", log.c_str());
			DriverLog("PError %s", log.c_str());
			hid_close(device);
			device = nullptr;
		}
		if(device2){
			hid_close(device2);
			device2 = nullptr;
		}
	}
};

static PimaxDirectUSB pimaxDirectUSB = {};

#ifdef PVR_EXISTS
void PvrThread();
std::atomic<bool> pvrThreadRunning;
static std::set<PimaxCommon*> sPimaxCommonInstances;
static std::mutex sPimaxCommonInstanceMutex;

static bool EnsurePvrSession(bool forceTryConnect = false) {
	if(s_info.connected && !forceTryConnect){
		// Don't attempt to connect to the PVR API if it already directly connected to the headset.
		// Also don't try re-connecting to the PVR API as the PVR thread will handle it
		return false;
	}
	if (!pvrThreadRunning.exchange(true)) {
		DriverLog("Starting PVR thread");
		std::thread pvrThread = std::thread(PvrThread);
		pvrThread.detach();
	}
	if (!s_pvr) {
		if (pvr_initialise(&s_pvr) != pvr_success) {
			return false;
		}
	}
	if (!s_pvrSession) {
		s_info.pvrConnected = false;
		if (pvr_createSession(s_pvr, &s_pvrSession) != pvr_success) {
			return false;
		}
	}
	
	shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
	
	if (s_pvrSession && !s_info.pvrConnected) {
		pvrHmdStatus hmdStatus = {};
		pvr_getHmdStatus(s_pvrSession, &hmdStatus);
		if (!(hmdStatus.ServiceReady && hmdStatus.HmdPresent && !hmdStatus.ShouldQuit)) {
			return false;
		} 
		
		const bool isOpenPortEnabled = pvr_getIntConfig(s_pvrSession, "no_render", 0);
		s_info.isOpenPortEnabled = isOpenPortEnabled;
		// if(!isOpenPortEnabled){
		// 	// Without open port, the headset will not be usable through PVR for tracking
		// 	return true;
		// }

		pvrHmdInfo info = {};
		pvr_getHmdInfo(s_pvrSession, &info);
		const std::string_view productName = info.ProductName;
		pvrDisplayInfo displayInfo = {};
		pvr_getEyeDisplayInfo(s_pvrSession, pvrEye_Left, &displayInfo);
		s_info.headsetType = TypeFromProduct(info.ProductId, productName, info.SerialNumber);
		if(s_info.headsetType == Config::HeadsetType::CrystalSuper50PPD){
			char codename[256] = {};
        	int len = pvr_getStringConfig(s_pvrSession, "hmd_codename", codename, sizeof(codename));
			std::string moduleName = codename + 12;
			DriverLog("Headset codename: %s, module: %s\n", codename, moduleName.c_str());
			if(moduleName == "50PPD QLED"){
				s_info.headsetType = Config::HeadsetType::CrystalSuper50PPD;
			}else if(moduleName == "57PPD QLED"){
				s_info.headsetType = Config::HeadsetType::CrystalSuper57PPD;
			}else if(moduleName == "Ultrawide"){
				s_info.headsetType = Config::HeadsetType::CrystalSuperUltrawide;
			}else if(moduleName == "Micro-OLED"){
				s_info.headsetType = Config::HeadsetType::CrystalSuperMicroOLED;
			}else{
				s_info.headsetType = Config::HeadsetType::None;
			}
			// I've seen some evidence that the EDIDs might have been changed at some point.
			if(s_info.headsetType){
				Config::BaseHeadsetConfig* config = driverConfig.ConfigFromHeadsetType(s_info.headsetType);
				if(config && displayInfo.edid_vid && config->edidVendorId != displayInfo.edid_vid){
					DriverLog("EDID vendor ID mismatch: expected %i, got %i\n", config->edidVendorId, displayInfo.edid_vid);
					config->edidVendorId = displayInfo.edid_vid;
					std::string maskedSerial = std::string(info.SerialNumber);
					if(maskedSerial.length() > 4){
						maskedSerial = maskedSerial.substr(0, maskedSerial.size() - 4) + "XXXX";
						DriverLog("Super headset serial: %s", maskedSerial.c_str());
					}
				}
			}
		}
		if (info.ProductId) {
			DriverLog("Detected PVR headset '%s' (%04x)%s", info.ProductName, info.ProductId, s_info.headsetType == Config::HeadsetType::None ? " - not supported" : "");
		}


		// Filter by: do we support the attached headset?
		s_info.pvrConnected = s_info.headsetType != Config::HeadsetType::None;
		if (s_info.pvrConnected) {
			pvrHmdTrackingStyle trackingStyle = pvrHmdTrackingStyle_Unknown;
			trackingStyle = (pvrHmdTrackingStyle)pvr_getTrackedDeviceIntProperty(
				s_pvrSession,
				pvrTrackedDevice_HMD,
				pvrTrackedDeviceProp_Prop_HmdTrackingStyle_Int,
				pvrHmdTrackingStyle_Unknown);
			// openport is required for SLAM tracking, and this will sometimes incorrectly match for Lighthouse headsets at startup, which will break non-openport launches.
			s_info.useSlamTracking = trackingStyle == pvrHmdTrackingStyle_InsideOutCameras && isOpenPortEnabled;

			DriverLog("Detected headset '%s' (%04x) with %s tracking", info.ProductName, info.ProductId, s_info.useSlamTracking ? "SLAM" : "Lighthouse");
			DriverLog("Panel Resolution: %ux%u (Orientation: %u deg)", displayInfo.width, displayInfo.height, displayInfo.eye_rotate * 90);
			s_info.resolutionX = displayInfo.width / 2;
			s_info.resolutionY = displayInfo.height;
				
			// Always query without parallel projection enabled.
			// The underlying driver will then (re)apply parallel projection if needed during Activate() and/or RunFrame().
			pvr_setIntConfig(s_pvrSession, "view_rotation_fix", 0);
			pvrEyeRenderInfo eyeInfo[pvrEye_Count] = {};
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Left, &eyeInfo[pvrEye_Left]);
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Right, &eyeInfo[pvrEye_Right]);
			s_info.ipd = PVR::Vector3f(eyeInfo[pvrEye_Left].HmdToEyePose.Position).Distance(eyeInfo[pvrEye_Right].HmdToEyePose.Position) * 1000;
			DriverLog("IPD: %.1f mm", s_info.ipd);
			s_info.cantingAngle = PVR::Quatf{ eyeInfo[pvrEye_Left].HmdToEyePose.Orientation }.Angle(eyeInfo[pvrEye_Right].HmdToEyePose.Orientation) / 2.f;
			s_info.cantingAngle *= 180 / 3.1415926f;
			DriverLog("Canting Angle: %.2f deg", s_info.cantingAngle);
			s_info.hasEyeTracking = HeadsetHasEyeTracking(s_info.headsetType);
			s_info.connected = true;
		}
	}
	return true;
}

void PvrThread(){
	bool wasEverSuccessful = false;
	while(true){
		EnsurePvrSession(true);
		
		if(s_pvrSession){
			pvrHmdStatus hmdStatus = {};
			pvrResult result;
			{
				shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
				result = pvr_getHmdStatus(s_pvrSession, &hmdStatus);
			}
			
			pvrButtonState = pvr_getIntConfig(s_pvrSession, "hmd_buttons", 0);
			
			pvrEyeRenderInfo eyeInfo[pvrEye_Count] = {};
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Left, &eyeInfo[pvrEye_Left]);
			pvr_getEyeRenderInfo(s_pvrSession, pvrEye_Right, &eyeInfo[pvrEye_Right]);
			float ipd = PVR::Vector3f(eyeInfo[pvrEye_Left].HmdToEyePose.Position).Distance(eyeInfo[pvrEye_Right].HmdToEyePose.Position) * 1000;
			if(ipd > 0){
				s_info.ipd = ipd;
			}

			// Call RunPvrBackground on all registered instances
			{
				std::lock_guard<std::mutex> lock(sPimaxCommonInstanceMutex);
				for (PimaxCommon* instance : sPimaxCommonInstances) {
					instance->RunPvrBackground();
				}
			}
			
			if (result != pvr_success || hmdStatus.ShouldQuit) {
				if (wasEverSuccessful) {
					DriverLog("Detected loss of connection to the headset");
				}
				std::lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
				if (wasEverSuccessful) {
					DriverLog("Destroying PVR session");
				}
				pvr_destroySession(s_pvrSession);
				// pvr_shutdown(s_pvr);
				s_info.pvrConnected = false;
				s_pvrSession = {};
				// s_pvr = {};
				wasEverSuccessful = false;
			} else {
				wasEverSuccessful = true;
			}

			SleepMs(20);
		}else{
			SleepMs(3000);
		}
		// bool anyHeadsetConnected = vr::VRProperties()->TrackedDeviceToPropertyContainer(0) != 0;
		if(!s_info.connected && CustomHeadsetDeviceProvider::hasHeadsetConnected){
			// some other non-Pimax headset is connected so stop the thread as it serves no purpose
			DriverLog("PVR thread exiting");
			break;
		}
	}
}
#endif // PVR_EXISTS

PimaxInfo PimaxCommon::GetInfo() {
#ifdef PVR_EXISTS
	EnsurePvrSession();
#endif
	return s_info;
}

bool PimaxCommon::TryDirectConnection() {
	// Attempt a direct connection if it's already directly connected or if the PVR session has never been connected.
	if(s_info.directConnected || (s_info.headsetType == Config::HeadsetType::None
#ifdef PVR_EXISTS
		&& (!EnsurePvrSession() || !s_info.isOpenPortEnabled)
#endif
	)){
		s_info.directConnected |= pimaxDirectUSB.Start();
		s_info.connected |= s_info.directConnected;
	}
	return s_info.directConnected;
}

bool PimaxCommon::IsPimaxLighthouseDevice(std::string_view model, std::string_view manufacturer){
	return model.find("Pimax") != std::string::npos || (model == "REF-HMD" && manufacturer.find("Pimax") != std::string::npos);
}

bool PimaxCommon::IsLighthouseHeadsetConnected(){
	PimaxInfo info = PimaxCommon::GetInfo();
	// useSlamTracking is unreliable early in the startup of the pi_server, even for Lighthouse headsets, so instead always allow Lighthouse devices
	return info.headsetType && driverConfig.ConfigFromHeadsetType(info.headsetType)->enable && info.connected; //&& (s_info.directConnected || !info.useSlamTracking);
}

bool PimaxCommon::IsSlamHeadsetConnected(){
	PimaxInfo info = PimaxCommon::GetInfo();
	return info.headsetType && driverConfig.ConfigFromHeadsetType(info.headsetType)->enable && info.connected && info.isOpenPortEnabled && info.useSlamTracking;
}

#ifdef PVR_EXISTS
pvrSessionHandle PimaxCommon::GetPvrSession(bool forceTryConnect) {
	EnsurePvrSession(forceTryConnect);
	return s_pvrSession;
}

double PimaxCommon::GetPvrTime() {
	return pvr_getTimeSeconds(s_pvr);
}
#endif

Config::BaseHeadsetConfig& PimaxCommon::PatchConfig(Config::BaseHeadsetConfig& config) {
	if (config.resolutionX == 0 || config.resolutionY == 0) {
		config.resolutionX = GetInfo().resolutionX;
		config.resolutionY = GetInfo().resolutionY;
	}
	if (config.hardwareIpd) {
		config.ipd = GetInfo().ipd;
	}
	if (config.autoEyeRotation) {
		if (config.distortionProfile == "Pimax Builtin" && config.parallelProjection) {
			config.eyeRotation = 0;
		}
		else {
			config.eyeRotation = GetInfo().cantingAngle;
		}
	}
	return config;
}

Config::PimaxHeadsetConfig& PimaxCommon::GetHeadsetConfig(){
	Config::PimaxHeadsetConfig* config = dynamic_cast<Config::PimaxHeadsetConfig*>(driverConfig.ConfigFromHeadsetType(GetInfo().headsetType));
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &driverConfig.dreamAir;
	}
	PatchConfig(*config);
	return *config;
}

Config::PimaxHeadsetConfig& PimaxCommon::GetHeadsetConfigOld(){
	Config::PimaxHeadsetConfig* config = dynamic_cast<Config::PimaxHeadsetConfig*>(driverConfigOld.ConfigFromHeadsetType(GetInfo().headsetType));
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &driverConfigOld.dreamAir;
	}
	PatchConfig(*config);
	return *config;
}

Config::PimaxHeadsetConfig& PimaxCommon::GetHeadsetConfigDefault(){
	Config::PimaxHeadsetConfig* config = dynamic_cast<Config::PimaxHeadsetConfig*>(defaultDriverConfig.ConfigFromHeadsetType(GetInfo().headsetType));
	if(!config){
		// fallback to the Dream Air to avoid null pointer
		config = &defaultDriverConfig.dreamAir;
	}
	return *config;
}

#ifdef PVR_EXISTS

PimaxCommon::PimaxCommon() {
	// Register this instance in the global set
	{
		std::lock_guard<std::mutex> lock(sPimaxCommonInstanceMutex);
		sPimaxCommonInstances.insert(this);
	}
	// Cache useful immutable state.
	shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
	pvr_getHmdInfo(GetPvrSession(), &hmdInfo);
}

PimaxCommon::~PimaxCommon() {
	// Remove this instance from the global set
	std::lock_guard<std::mutex> lock(sPimaxCommonInstanceMutex);
	sPimaxCommonInstances.erase(this);
}

bool PimaxCommon::CheckPvrDeviceLost() {
	return !s_info.pvrConnected;
}

bool PimaxCommon::HasEyeTracking() const {
	return s_info.hasEyeTracking;
}

void PimaxCommon::StartEyeTracking() {
	if (s_pvrSession && !eyeTrackingRunning.exchange(true)) {
		DriverLog("Starting eye tracking thread");
		eyeTrackingThread = std::thread(&PimaxCommon::EyeTrackingThread, this);
	}
}

void PimaxCommon::StopEyeTracking() {
	if (eyeTrackingRunning.exchange(false) && eyeTrackingThread.joinable()) {
		DriverLog("Stopping eye tracking thread");
		eyeTrackingThread.join();
		eyeTrackingThread = {};
	}
}

void PimaxCommon::SetVisibilityMeshes() {
	vr::CVRHiddenAreaHelpers helpers = { vr::VRPropertiesRaw() };
	shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
	for (int eye = 0; eye < pvrEye_Count; eye++) {
		std::vector<vr::HmdVector2_t> vertices;
		size_t count;

		count = pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_HiddenArea, nullptr, 0);
		vertices.resize(count);
		pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_HiddenArea,
			(pvrVector2f*)vertices.data(), (unsigned int)vertices.size());
		helpers.SetHiddenArea((vr::EVREye)eye, vr::k_eHiddenAreaMesh_Standard, vertices.data(), (uint32_t)vertices.size());

		count = pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_VisibleArea, nullptr, 0);
		vertices.resize(count);
		pvr_getEyeHiddenAreaMesh(GetPvrSession(), (pvrEyeType)eye, pvrHiddenAreaMesh_VisibleArea,
			(pvrVector2f*)vertices.data(), (unsigned int)vertices.size());
		helpers.SetHiddenArea((vr::EVREye)eye, vr::k_eHiddenAreaMesh_Inverse, vertices.data(), (uint32_t)vertices.size());
	}
}

void PimaxCommon::PollMagicAttach() {
	// Detect if Pimax LibMagic (DFR injector) was enabled after a scene application started and re-assert the
	// PID of the current scene application.
	const bool wasLibMagicEnabled = isLibMagicEnabled;
	shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
	isLibMagicEnabled = pvr_getIntConfig(GetPvrSession(), "enable_foveated_rendering", 0);
	if (isLibMagicEnabled != wasLibMagicEnabled) {
		SetSceneApplicationProcess(lastSceneApplicationPid);
	}
}

void PimaxCommon::SetSceneApplicationProcess(uint32_t pid) {
	if (pid) {
		shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
		// Signal Pimax Play to perform a MagicAttach (DFR injector) when a new scene app started.
		pvr_setIntConfig(GetPvrSession(), "openvr_client_changed", pid);
	}
	lastSceneApplicationPid = pid;
}
#endif

void PimaxCommon::GetHmdButtonsState(bool& systemButton, bool& doubleTap) {
	shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
	HmdButton hmdButtonsState = HmdButton::Button_None;
	if(s_info.pvrConnected){
		hmdButtonsState = (HmdButton)((int)hmdButtonsState | pvrButtonState);
	}
	if(s_info.directConnected){
		hmdButtonsState = (HmdButton)((int)hmdButtonsState | directButtonState);
	}
	const auto isButtonPressed = [&hmdButtonsState](const HmdButton button) {
		return ((int)hmdButtonsState & (int)button) == (int)button;
	};
	systemButton = isButtonPressed(HmdButton::Button_VolumeUp) && isButtonPressed(HmdButton::Button_VolumeDown);
	doubleTap = isButtonPressed(HmdButton::Button_DoubleTap);
}

#ifdef PVR_EXISTS
void PimaxCommon::EyeTrackingThread() {
	const HANDLE timer = CreateWaitableTimer(nullptr, false, nullptr);
	const LARGE_INTEGER noDelay = {};
	const LONG periodMs = 1000 / 120; // Approx 120Hz
	SetWaitableTimer(timer, &noDelay, periodMs, nullptr, nullptr, true);

	while (true) {
		WaitForSingleObject(timer, 100);
		if (!eyeTrackingRunning) {
			break;
		}

		pvrEyeTrackingInfo eyeTrackingInfo = {};
		shared_lock_guard<std::shared_mutex> lock(PimaxCommon::pvrLock);
		pvr_getEyeTrackingInfo(GetPvrSession(), GetPvrTime(), &eyeTrackingInfo);
		if (eyeTrackingInfo.TimeInSeconds) {
			// Convert PVR eye tracking data to angles for EyeTrackingOutput.
			// PVR provides gaze as tangent values, convert to angles in radians.
			const float leftAngleX = atanf(eyeTrackingInfo.GazeTan[pvrEye_Left].x);
			const float leftAngleY = atanf(eyeTrackingInfo.GazeTan[pvrEye_Left].y);
			const float rightAngleX = atanf(eyeTrackingInfo.GazeTan[pvrEye_Right].x);
			const float rightAngleY = atanf(eyeTrackingInfo.GazeTan[pvrEye_Right].y);
			eyeTrackingOutput.SetEyeTrackingData(leftAngleX, leftAngleY, rightAngleX, rightAngleY);
		}
	}

	CloseHandle(timer);

	DriverLog("Eye tracking thread stopped");
}
#endif // PVR_EXISTS

