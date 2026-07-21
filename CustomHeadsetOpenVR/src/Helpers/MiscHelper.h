#pragma once

#include <string>

// Compares two version strings and returns true if 'latest' is newer than 'current'.
// Handles optional leading 'v'/'V' prefix, numeric and alphanumeric parts separated by '.' or '-'.
bool IsNewVersion(const std::string& current, const std::string& latest);

// Looks up the DisplayVersion from the Windows registry Uninstall keys for a program matching displayName.
// Searches both 32-bit and 64-bit registry views. Returns empty string if not found.
std::string GetInstalledProgramVersion(const std::string& displayName);