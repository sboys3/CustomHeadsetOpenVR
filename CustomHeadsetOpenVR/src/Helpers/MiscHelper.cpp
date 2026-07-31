#include "MiscHelper.h"

#include <algorithm>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#undef min
#undef max

static std::vector<std::string> SplitVersion(const std::string& version) {
	std::vector<std::string> parts;
	std::string::size_type start = 0;
	std::string::size_type end = version.find_first_of(".-");

	while (end != std::string::npos) {
		parts.push_back(version.substr(start, end - start));
		start = end + 1;
		end = version.find_first_of(".-", start);
	}

	parts.push_back(version.substr(start));
	return parts;
}

static bool IsNumeric(const std::string& part) {
	if (part.empty()) return false;
	for (char c : part) {
		if (c < '0' || c > '9') return false;
	}
	return true;
}

bool IsNewVersion(const std::string& current, const std::string& latest) {
	// Strip leading 'v' or 'V'
	std::string cStr = current;
	std::string lStr = latest;

	if (!cStr.empty() && (cStr[0] == 'v' || cStr[0] == 'V')) {
		cStr = cStr.substr(1);
	}
	if (!lStr.empty() && (lStr[0] == 'v' || lStr[0] == 'V')) {
		lStr = lStr.substr(1);
	}

	auto cParts = SplitVersion(cStr);
	auto lParts = SplitVersion(lStr);

	size_t maxLength = std::max(cParts.size(), lParts.size());

	for (size_t i = 0; i < maxLength; i++) {
		std::string cPart = i < cParts.size() ? cParts[i] : "";
		std::string lPart = i < lParts.size() ? lParts[i] : "";

		bool cIsNumeric = IsNumeric(cPart);
		bool lIsNumeric = IsNumeric(lPart);

		if (!cIsNumeric || !lIsNumeric) {
			// Compare as strings
			if (lPart.empty()) {
				// Text is missing from latest, so it is no longer a pre-release
				return true;
			}
			if (cPart.empty()) {
				// Text is missing from current, so latest is an older pre-release
				return false;
			}
			if (lPart > cPart) {
				return true;
			}
			if (cPart > lPart) {
				return false;
			}
			continue;
		}

		// Compare as numbers
		int64_t cNum = std::stoll(cPart);
		int64_t lNum = std::stoll(lPart);

		if (lNum > cNum) {
			return true;
		}
		if (cNum > lNum) {
			return false;
		}
	}

	return false;
}

#ifdef _WIN32
static std::string WStringToString(const std::wstring& ws) {
	if (ws.empty()) return "";
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.length(), nullptr, 0, nullptr, nullptr);
	if (sizeNeeded <= 0) return "";
	std::string strTo(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.length(), &strTo[0], sizeNeeded, nullptr, nullptr);
	return strTo;
}

static std::wstring StringToWString(const std::string& s) {
	if (s.empty()) return L"";
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), nullptr, 0);
	if (sizeNeeded <= 0) return L"";
	std::wstring wsTo(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), &wsTo[0], sizeNeeded);
	return wsTo;
}

static bool ReadRegistryStringValue(HKEY hKey, const std::wstring& valueName, std::wstring& outValue) {
	DWORD bufferSize = 4096;
	std::vector<wchar_t> buffer(bufferSize, 0);
	DWORD type = 0;
	LONG result = RegQueryValueExW(hKey, valueName.c_str(), 0, &type, reinterpret_cast<LPBYTE>(&buffer[0]), &bufferSize);

	if (result == ERROR_SUCCESS && type == REG_SZ) {
		outValue = std::wstring(buffer.data(), bufferSize / sizeof(wchar_t) - 1);
		return true;
	}
	return false;
}

static bool ContainsInsensitive(const std::wstring& haystack, const std::wstring& needle) {
	std::wstring lowerHay = haystack;
	std::transform(lowerHay.begin(), lowerHay.end(), lowerHay.begin(), towlower);
	std::wstring lowerNeedle = needle;
	std::transform(lowerNeedle.begin(), lowerNeedle.end(), lowerNeedle.begin(), towlower);
	return lowerHay.find(lowerNeedle) != std::wstring::npos;
}

std::string GetInstalledProgramVersion(const std::string& displayName) {
	std::wstring searchName = StringToWString(displayName);
	std::string resultVersion;

	const std::vector<std::wstring> registryPaths = {
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
	};

	for (const auto& path : registryPaths) {
		HKEY hUninstallKey;
		LONG openResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hUninstallKey);
		if (openResult != ERROR_SUCCESS) {
			continue;
		}

		DWORD index = 0;
		DWORD subkeyNameSize = 256;
		std::vector<wchar_t> subkeyName(subkeyNameSize, 0);

		while (RegEnumKeyExW(hUninstallKey, index, &subkeyName[0], &subkeyNameSize, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
			HKEY hSubKey;
			LONG subKeyResult = RegOpenKeyExW(hUninstallKey, subkeyName.data(), 0, KEY_READ, &hSubKey);
			if (subKeyResult == ERROR_SUCCESS) {
				std::wstring displayNameValue;
				if (ReadRegistryStringValue(hSubKey, L"DisplayName", displayNameValue) && !displayNameValue.empty()) {
					if (ContainsInsensitive(displayNameValue, searchName)) {
						std::wstring versionValue;
						if (ReadRegistryStringValue(hSubKey, L"DisplayVersion", versionValue)) {
							resultVersion = WStringToString(versionValue);
						}
					}
				}
				RegCloseKey(hSubKey);
			}

			subkeyNameSize = 256;
			std::fill(subkeyName.begin(), subkeyName.end(), 0);
			index++;
		}

		RegCloseKey(hUninstallKey);
	}

	return resultVersion;
}
#else
std::string GetInstalledProgramVersion(const std::string& displayName) {
	return "";
}
#endif

void* LibOpen(const std::string& path) {
	#ifdef _WIN32
	return LoadLibraryA(path.c_str());
	#else
	return dlopen(path.c_str(), RTLD_NOW);
	#endif
}

void LibClose(void* handle) {
	#ifdef _WIN32
	FreeLibrary((HMODULE)handle);
	#else
	dlclose(handle);
	#endif
}

void* LibAddress(void* handle, const std::string& name) {
	#ifdef _WIN32
	return (void*)GetProcAddress((HMODULE)handle, name.c_str());
	#else
	return dlsym(handle, name.c_str());
	#endif
}