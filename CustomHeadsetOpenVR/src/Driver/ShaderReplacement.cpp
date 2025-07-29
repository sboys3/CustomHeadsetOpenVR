#include "ShaderReplacement.h"
#include "DriverLog.h"
#include "Hooking/Hooking.h"
#include "../Config/ConfigLoader.h"
#include <mutex>
#include <map>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <fstream>

#include "../../../ThirdParty/easywsclient/easywsclient.hpp"

#include "d3d11.h"
#include "d3dcompiler.h"
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D3DCompiler.lib")

// this could probably use macros to get the current project location instead of hard coding it for my computer
std::string shaderDevPath = "C:/Users/Admin/Desktop/stuff/projects/meganex/CustomHeadsetOpenVR/CustomHeadsetOpenVR/DriverFiles/resources/shaders/d3d11/";

std::string getShaderPath(){
	std::string shaderPath = driverConfigLoader.info.driverResources + "shaders/d3d11/";
	if(std::filesystem::exists(shaderDevPath)){
		// pull from my dev path instead
		shaderPath = shaderDevPath;
	}
	return shaderPath;
}

static Hook<void*(*)(ID3D11Device*, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader)> 
	CreatePixelShaderHook("ID3D11Device::CreatePixelShader");

// holder for bytecode
struct Bytecode{
	char* data;
	size_t length;
};

// map of functions to replace shader bytecode with based on the first 32 bytes of the shader bytecode
static std::map<std::string, Bytecode(*)()> shaderReplacements;

// debug shaders by logging the first 32 bytes of the shader bytecode
void LogShaderIdentifier(std::string identifierString, size_t length){
	DriverLog("Shader identifier: %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c, length: %zu", identifierString[0], identifierString[1], identifierString[2], identifierString[3], identifierString[4], identifierString[5], identifierString[6], identifierString[7], identifierString[8], identifierString[9], identifierString[10], identifierString[11], identifierString[12], identifierString[13], identifierString[14], identifierString[15], identifierString[16], identifierString[17], identifierString[18], identifierString[19], identifierString[20], identifierString[21], identifierString[22], identifierString[23], identifierString[24], identifierString[25], identifierString[26], identifierString[27], identifierString[28], identifierString[29], identifierString[30], identifierString[31], length);
}

int dumpId = 0;

// function that hooks the CreatePixelShader function of the ID3D11Device interface
static void DetourCreatePixelShader(ID3D11Device* _this, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader){
 	// DriverLog("DetourCreatePixelShader Called");
	if(pShaderBytecode == nullptr || BytecodeLength < 32){
		CreatePixelShaderHook.originalFunc(_this, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
		return;
	}
	// identify shader by first 32 bytes which contains includes a hash
	std::string identifierString((char*)pShaderBytecode, 32);
	// LogShaderIdentifier(identifierString, BytecodeLength);
	if(driverConfig.customShader.enable && shaderReplacements.find(identifierString) != shaderReplacements.end()){
		Bytecode newBytecode = shaderReplacements[identifierString]();
		// DriverLog("==================================");
		if(newBytecode.data == nullptr || newBytecode.length == 0){
			DriverLog("Failed to get new bytecode");
		}else{
			DriverLog("Replacing shader bytecode");
			CreatePixelShaderHook.originalFunc(_this, newBytecode.data, newBytecode.length, pClassLinkage, ppPixelShader);
			delete[] newBytecode.data;
			return;
		}
	}
	// dump shader bytecode to file
	// std::string dumpPath = driverConfigLoader.info.driverResources + "shaders/dump/" + std::to_string(dumpId++) + ".fxo";
	// FILE* dumpFile = fopen(dumpPath.c_str(), "wb+");
	// if(dumpFile){
	// 	fwrite(pShaderBytecode, 1, BytecodeLength, dumpFile);
	// 	fclose(dumpFile);
	// 	DriverLog("Dumped shader bytecode to %s", dumpPath.c_str());
	// }else{
	// 	DriverLog("Failed to dump shader bytecode to %s", dumpPath.c_str());
	// }
	// if no replacement is found, call the original function with the original bytecode
	CreatePixelShaderHook.originalFunc(_this, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
}

std::string ConvertWideToUtf8(const std::wstring& wstr){
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], (int)count, NULL, NULL);
    return str;
}

std::wstring ConvertUtf8ToWide(const std::string& str){
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], count);
    return wstr;
}


static std::map<ConfigLoader::HeadsetType, std::vector<double>> srgbColorCorrectionMatrices = {
	{ConfigLoader::HeadsetType::MeganeX8K, {
		// convert sRGB into the section of the dci-p3 color space it occupies
		0.8224619687143621, 0.17753803128563772, 0.0, 
		0.033194198850961636, 0.9668058011490385, -1.3877787807814457e-17, 
		0.01708263072112004, 0.07239744066396342, 0.9105199286149167
	}},
};

// compile the new distortion shader from source
Bytecode DistortionShader(bool muraCorrection = false, bool noDistortion = false){
	if(!IsCustomShaderEnabled()){
		// don't replace
		return {nullptr, 0};
	}
	
	
	std::string fullPath = getShaderPath() + "distort_ps_layered.hlsl";
	// FILE* file = fopen(fullPath.c_str(), "rb");
	// if(!file){
	// 	DriverLog("Failed to open shader file: %s", fullPath.c_str());
	// 	return {nullptr, 0};
	// }
	// fseek(file, 0, SEEK_END);
	// size_t length = ftell(file);
	// fseek(file, 0, SEEK_SET);
	// char* data = new char[length];
	// fread(data, 1, length, file);
	// fclose(file);
	
	// ID3DBlob* blob;
	// // std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	// // std::wstring shaderPath = converter.from_bytes(fullPath);
	// std::wstring shaderPath = std::wstring(fullPath.begin(), fullPath.end());
	// if(FAILED(D3DReadFileToBlob(shaderPath.c_str(), &blob))){
	// 	DriverLog("Failed to read shader file: %s", fullPath.c_str());
	// 	return {nullptr, 0};
	// }
	// char* data = new char[blob->GetBufferSize()];
	// memcpy(data, blob->GetBufferPointer(), blob->GetBufferSize());
	// size_t length = blob->GetBufferSize();
	// blob->Release();
	
	
	// create defines for shader settings
	D3D_SHADER_MACRO defines[30] = {};
	int definesCount = 0;
	if(driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::MeganeX8K){
		defines[definesCount++] = {"MEGANEX8K", "1"};
		if(driverConfig.customShader.subpixelShift && driverConfig.meganeX8K.subpixelShift != 0 && driverConfig.meganeX8K.resolutionY == 3552){
			// only do this if the subpixel shift is not zero and it is running at full resolution
			defines[definesCount++] = {"SUBPIXEL_SHIFT_MEGANEX8K", "1"};
		}
	}
	if(driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::Vive){
		defines[definesCount++] = {"VIVE", "1"};
		if(driverConfig.customShader.subpixelShift){
			defines[definesCount++] = {"SUBPIXEL_SHIFT_VIVE", "1"};
		}
	}
	double contrastMultiplier = driverConfig.customShader.contrast / 50.0;
	std::string contrastMultiplierString = std::to_string(contrastMultiplier);
	if(contrastMultiplier != 1){
		defines[definesCount++] = {"CONTRAST_MULTIPLIER", contrastMultiplierString.c_str()};
	}
	double contrastOffset = driverConfig.customShader.contrastMidpoint / 100.0;
	contrastOffset = -contrastOffset  * contrastMultiplier + contrastOffset;
	std::string contrastOffsetString = std::to_string(contrastOffset);
	if(contrastOffset != 0){
		defines[definesCount++] = {"CONTRAST_OFFSET", contrastOffsetString.c_str()};
	}
	if(driverConfig.customShader.contrastLinear){
		defines[definesCount++] = {"CONTRAST_LINEAR", "1"};
	}
	std::string contrastMultiplierLeft = "";
	std::string contrastOffsetLeft = "";
	std::string contrastMultiplierRight = "";
	std::string contrastOffsetRight = "";
	if(driverConfig.customShader.contrastPerEye){
		double contrastMultiplierLeftValue = driverConfig.customShader.contrastLeft / 50.0;
		double contrastOffsetLeftValue = driverConfig.customShader.contrastMidpointLeft / 100.0;
		contrastOffsetLeftValue = -contrastOffsetLeftValue  * contrastMultiplierLeftValue + contrastOffsetLeftValue;
		double contrastMultiplierRightValue = driverConfig.customShader.contrastRight / 50.0;
		double contrastOffsetRightValue = driverConfig.customShader.contrastMidpointRight / 100.0;
		contrastOffsetRightValue = -contrastOffsetRightValue  * contrastMultiplierRightValue + contrastOffsetRightValue;
		if(contrastMultiplierLeftValue != 1){
			contrastMultiplierLeft = std::to_string(contrastMultiplierLeftValue);
			defines[definesCount++] = {"CONTRAST_MULTIPLIER_LEFT", contrastMultiplierLeft.c_str()};
		}
		if(contrastOffsetLeftValue != 0){
			contrastOffsetLeft = std::to_string(contrastOffsetLeftValue);
			defines[definesCount++] = {"CONTRAST_OFFSET_LEFT", contrastOffsetLeft.c_str()};
		}
		if(contrastMultiplierRightValue != 1){
			contrastMultiplierRight = std::to_string(contrastMultiplierRightValue);
			defines[definesCount++] = {"CONTRAST_MULTIPLIER_RIGHT", contrastMultiplierRight.c_str()};
		}
		if(contrastOffsetRightValue != 0){
			contrastOffsetRight = std::to_string(contrastOffsetRightValue);
			defines[definesCount++] = {"CONTRAST_OFFSET_RIGHT", contrastOffsetRight.c_str()};
		}
		if(driverConfig.customShader.contrastPerEyeLinear){
			defines[definesCount++] = {"CONTRAST_PER_EYE_LINEAR", "1"};
		}
	}
	double chroma = driverConfig.customShader.chroma / 50.0;
	std::string chromaString = std::to_string(chroma);
	if(chroma != 1){
		defines[definesCount++] = {"CHROMA", chromaString.c_str()};
	}
	std::string gammaString = std::to_string(driverConfig.customShader.gamma);
	if(driverConfig.customShader.gamma != 2.2){
		defines[definesCount++] = {"GAMMA", gammaString.c_str()};
	}
	if(muraCorrection){
		defines[definesCount++] = {"MURA_CORRECTION", "1"};
		if(driverConfig.customShader.disableMuraCorrection){
			defines[definesCount++] = {"DISABLE_MURA_CORRECTION", "1"};
		}
	}
	if(driverConfig.customShader.disableBlackLevels){
		defines[definesCount++] = {"DISABLE_BLACK_LEVELS", "1"};
	}
	std::string colorMatrixString = "";
	if(driverConfig.customShader.srgbColorCorrection){
		std::vector<double>* colorMatrix = nullptr;
		if(srgbColorCorrectionMatrices.find(driverConfigLoader.info.connectedHeadset) != srgbColorCorrectionMatrices.end()){
			colorMatrix = &srgbColorCorrectionMatrices[driverConfigLoader.info.connectedHeadset];
		}
		if(driverConfig.customShader.srgbColorCorrectionMatrix.size() == 9){
			colorMatrix = &driverConfig.customShader.srgbColorCorrectionMatrix;
		}
		if(colorMatrix){
			for(int i = 0; i < 9; i++){
				colorMatrixString += std::to_string((*colorMatrix)[i]);
				if(i < 8){
					colorMatrixString += ", ";
				}
			}
			defines[definesCount++] = {"COLOR_CORRECTION_MATRIX", colorMatrixString.c_str()};
		}
	}
	if(driverConfig.customShader.lensColorCorrection){
		defines[definesCount++] = {"LENS_COLOR_CORRECTION", "1"};
	}
	if(driverConfig.customShader.dither10Bit){
		defines[definesCount++] = {"DITHER_10BIT", "1"};
	}
	if(noDistortion){
		defines[definesCount++] = {"NO_DISTORTION", "1"};
		defines[definesCount++] = {"NO_LAYER", "1"};
	}
	if(std::filesystem::exists(getShaderPath() + "distort_ps_layered_after_test.hlsl")){
		defines[definesCount++] = {"AFTER_TEST", "1"};
	}
	
	defines[definesCount++] = {nullptr, nullptr}; // end of array
	
	
	std::string errorPath = fullPath + "_error" + (muraCorrection ? "_mura" : "") + (noDistortion ? "_nd" : "") + ".txt";
	
	// compile shader from hlsl using D3DCompileFromFile
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr; 
	if(FAILED(D3DCompileFromFile(
		ConvertUtf8ToWide(fullPath).c_str(),
		defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		"ps_5_0",
		0,
		0,
		&shaderBlob,
		&errorBlob
	))){
		DriverLog("Failed to compile shader file: %s", fullPath.c_str());
		if(errorBlob){
			DriverLog("Error: %s", (char*)errorBlob->GetBufferPointer());
			// output to file beside shader
			FILE* errorFile = fopen(errorPath.c_str(), "wb+");
			if(errorFile){
				fwrite(errorBlob->GetBufferPointer(), 1, errorBlob->GetBufferSize() - 1, errorFile);
				fclose(errorFile);
			}
		}
		return {nullptr, 0};
	}else{
		DriverLog("Successfully compiled shader file: %s", fullPath.c_str());
		Bytecode bytecode = {new char[shaderBlob->GetBufferSize()], shaderBlob->GetBufferSize()};
		memcpy(bytecode.data, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		shaderBlob->Release();
		// delete error file
		remove(errorPath.c_str());
		return bytecode;
	}
}


// same as DistortionShader
Bytecode DistortionShaderPlain(){ 
	return DistortionShader();
}

// same as DistortionShader but with mura correction enabled
Bytecode DistortionShaderMuraCorrection(){
	if(driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::MeganeX8K){
		// don't compile for headsets that will not use it
		return {nullptr, 0};
	}
	return DistortionShader(true);
}

// same as DistortionShader but with no distortion enabled
Bytecode DistortionShaderNoDistortion(){
	if(driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::MeganeX8K || driverConfigLoader.info.connectedHeadset == ConfigLoader::HeadsetType::Vive){
		// don't compile for headsets that will not use it
		return {nullptr, 0};
	}
	return DistortionShader(false, true);
}



// get first 32 bytes of an existing shader for identification
std::string GetExistingShaderIdentifier(std::string name){
	std::string fullPath = driverConfigLoader.info.steamvrResources + "shaders/d3d11/" + name;
	FILE* file = fopen(fullPath.c_str(), "rb");
	if(!file){
		DriverLog("Failed to open shader file: %s", fullPath.c_str());
		return "";
	}
	DriverLog("Reading shader file: %s", fullPath.c_str());
	char buffer[32];
	fread(buffer, 1, 32, file);
	fclose(file);
	// ID3DBlob* blob;
	// // std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	// // std::wstring shaderPath = converter.from_bytes(fullPath);
	// std::wstring shaderPath = std::wstring(fullPath.begin(), fullPath.end());
	// if(FAILED(D3DReadFileToBlob(shaderPath.c_str(), &blob))){
	// 	DriverLog("Failed to read shader file: %s", fullPath.c_str());
	// 	return "";
	// }
	// if(blob->GetBufferSize() < 32){
	// 	DriverLog("Shader file too small: %s", fullPath.c_str());
	// 	blob->Release();
	// 	return "";
	// }
	// char buffer[32];
	// memcpy(buffer, blob->GetBufferPointer(), 32);
	// blob->Release();
	return std::string(buffer, 32);
}

void ShaderReplacement::Initialize(){
	if(started){
		return;
	}
	started = true;
	DriverLog("ShaderReplacement::Initialize called");
	
	// get and shim ID3D11Device
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	HRESULT hr = D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&device,
		NULL,
		&context
	);            // return the context
	if(FAILED(hr)){
		DriverLog("ShaderReplacement::Initialize Failed to create D3D11 device and context");
		return;
	}else{
		DriverLog("ShaderReplacement::Initialize Successfully created D3D11 device and context");
	}
	// device->CreatePixelShader
	auto err = MH_Initialize();
	if (err == MH_OK)
	{
		CreatePixelShaderHook.CreateHookInObjectVTable(device, 15, &DetourCreatePixelShader);
		IHook::Register(&CreatePixelShaderHook);
	}
	
	device->Release();
	context->Release();
	
	
	DriverLog("ShaderReplacement::Initialize loading shader replacement table");
	// LogShaderIdentifier(GetExistingShaderIdentifier("distort_ps_layered.fxo"), 32);
	shaderReplacements[GetExistingShaderIdentifier("distort_ps_layered.fxo")] = DistortionShaderPlain;
	shaderReplacements[GetExistingShaderIdentifier("distort_ps_layered_mc.fxo")] = DistortionShaderMuraCorrection;
	// this is for headsets with custom compositors
	// this only works for games when there is an overlay open, otherwise there the textures never seem to pass through a shader
	// a transparent overlay could be opened to force this
	shaderReplacements[GetExistingShaderIdentifier("distort_ps_achromatic_nd.fxo")] = DistortionShaderNoDistortion;
	
	#ifdef _WIN32
		// startup winsock for websocket connections
		WSADATA wsaData;
		int wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if(wsaStartupResult){
			printf("WSAStartup Failed %i.\n", wsaStartupResult);
		}
	#endif
	
	// start threads
	
	std::thread watchShadersThread(&ShaderReplacement::WatchShadersThread, this);
	watchShadersThread.detach();
	
	std::thread checkSettingsThread(&ShaderReplacement::CheckSettingsThread, this);
	checkSettingsThread.detach();
	
	
	// reloading shaders immediately does not seem to work so try a few times with delays
	std::this_thread::sleep_for(std::chrono::seconds(5));
	if(driverConfig.customShader.enable){
		ReloadShaders();
	}
	std::this_thread::sleep_for(std::chrono::seconds(5));
	if(driverConfig.customShader.enable){
		ReloadShaders();
	}
	std::this_thread::sleep_for(std::chrono::seconds(5));
	if(driverConfig.customShader.enable){
		ReloadShaders();
	}
}

void ShaderReplacement::ReloadShaders(){
	DriverLog("ShaderReplacement::ReloadShaders called");
	easywsclient::WebSocket* websocket = easywsclient::WebSocket::from_url("ws://127.0.0.1:27062/", "http://127.0.0.1:27062");
	if(websocket == nullptr){
		DriverLog("Failed to create websocket");
		return;
	}
	websocket->send("mailbox_send vrcompositor_mailbox {\"type\":\"shaders_force_reload\"}");
	websocket->poll();
	websocket->close();
	websocket->poll(); // final poll to close the connection
	delete websocket;
}



void ShaderReplacement::WatchShadersThread(){
	std::string shaderPath = getShaderPath();
	HANDLE hDir = CreateFileA(shaderPath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(hDir == INVALID_HANDLE_VALUE){
		DriverLog("Failed to open distortion shaders for watching: %d", GetLastError());
		return;
	}
	while(started){
		DWORD bytesReturned;
		char buffer[1024];
		FILE_NOTIFY_INFORMATION* pNotify;
		BOOL success = ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, NULL, NULL);
		if(!success){
			DriverLog("Failed to read directory changes: %d", GetLastError());
			break;
		}
		pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
		do{
			std::wstring fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t));
			if(fileName.find(L".hlsl") != std::wstring::npos && (pNotify->Action == FILE_ACTION_MODIFIED || pNotify->Action == FILE_ACTION_ADDED || pNotify->Action == FILE_ACTION_RENAMED_NEW_NAME)){
				DriverLog("Shader changed, reloading...");
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
				ReloadShaders();
				break;
			}
			pNotify = (FILE_NOTIFY_INFORMATION*)((char*)pNotify + pNotify->NextEntryOffset);
		}while(pNotify->NextEntryOffset != 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void ShaderReplacement::CheckSettingsThread(){
	while(started){
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		{
			std::lock_guard<std::mutex> lock(driverConfigLock);
			if(!driverConfig.hasBeenUpdated && !driverConfigLoader.info.hasBeenUpdated){
				continue;
			}
			driverConfig.hasBeenUpdated = false;
			driverConfigLoader.info.hasBeenUpdated = false;
			bool reloadShaders = false;
			bool isNowEnabled = IsCustomShaderEnabled();
			reloadShaders |= isNowEnabled != enabled;
			enabled = isNowEnabled;
			if(isNowEnabled){
				reloadShaders |= driverConfig.meganeX8K.subpixelShift != driverConfigOld.meganeX8K.subpixelShift;
				reloadShaders |= driverConfig.customShader.enable != driverConfigOld.customShader.enable;
				reloadShaders |= driverConfig.customShader.contrast != driverConfigOld.customShader.contrast;
				reloadShaders |= driverConfig.customShader.contrastMidpoint != driverConfigOld.customShader.contrastMidpoint;
				reloadShaders |= driverConfig.customShader.contrastLinear != driverConfigOld.customShader.contrastLinear;
				reloadShaders |= driverConfig.customShader.contrastPerEye != driverConfigOld.customShader.contrastPerEye;
				reloadShaders |= driverConfig.customShader.contrastPerEyeLinear != driverConfigOld.customShader.contrastPerEyeLinear;
				reloadShaders |= driverConfig.customShader.contrastLeft != driverConfigOld.customShader.contrastLeft;
				reloadShaders |= driverConfig.customShader.contrastMidpointLeft != driverConfigOld.customShader.contrastMidpointLeft;
				reloadShaders |= driverConfig.customShader.contrastRight != driverConfigOld.customShader.contrastRight;
				reloadShaders |= driverConfig.customShader.contrastMidpointRight != driverConfigOld.customShader.contrastMidpointRight;
				reloadShaders |= driverConfig.customShader.chroma != driverConfigOld.customShader.chroma;
				reloadShaders |= driverConfig.customShader.gamma != driverConfigOld.customShader.gamma;
				reloadShaders |= driverConfig.customShader.subpixelShift != driverConfigOld.customShader.subpixelShift;
				reloadShaders |= driverConfig.customShader.disableMuraCorrection != driverConfigOld.customShader.disableMuraCorrection;
				reloadShaders |= driverConfig.customShader.disableBlackLevels != driverConfigOld.customShader.disableBlackLevels;
				reloadShaders |= driverConfig.customShader.srgbColorCorrection != driverConfigOld.customShader.srgbColorCorrection;
				reloadShaders |= driverConfig.customShader.srgbColorCorrectionMatrix.size() != driverConfigOld.customShader.srgbColorCorrectionMatrix.size();
				reloadShaders |= driverConfig.customShader.lensColorCorrection != driverConfigOld.customShader.lensColorCorrection;
				reloadShaders |= driverConfig.customShader.dither10Bit != driverConfigOld.customShader.dither10Bit;
				if(driverConfig.customShader.srgbColorCorrectionMatrix.size() == 9 && driverConfigOld.customShader.srgbColorCorrectionMatrix.size() == 9){
					for(int i = 0; i < 9; i++){
						reloadShaders |= driverConfig.customShader.srgbColorCorrectionMatrix[i] != driverConfigOld.customShader.srgbColorCorrectionMatrix[i];
					}
				}
			}
			
			if(reloadShaders){
				DriverLog("Shader settings changed, reloading...");
				ReloadShaders();
			}
		}
	}
}