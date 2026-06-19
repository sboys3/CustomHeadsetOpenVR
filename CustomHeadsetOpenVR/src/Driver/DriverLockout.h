#pragma once

// Driver lockout functionality for vendor-based driver management
// When a vendor-specific driver is loaded, it checks if the vendor-neutral driver
// (CustomHeadsetOpenVR) is enabled. If so, the vendor driver will be locked out.
// This prevents conflicts between vendor-specific and vendor-neutral drivers.
// Reads openvrpaths.vrpath and steamvr.vrsettings directly (same as the GUI).

// Check if the vendor-neutral driver (CustomHeadsetOpenVR) is enabled in SteamVR settings
// Returns true if the neutral driver is enabled
// Returns false if the neutral driver is disabled, blocked, or not present in settings
bool IsNeutralDriverEnabled();

// Check if a specific driver is enabled in SteamVR settings
// Returns true if enabled (or present with enable not specified, defaulting to enabled)
// Returns false if explicitly disabled, blocked by safe mode, or not present in settings
bool IsDriverEnabled(const char* driverName);

