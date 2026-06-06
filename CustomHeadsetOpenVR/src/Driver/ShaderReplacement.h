#pragma once
#include "../Config/Config.h"

// replaces shaders within vrcompositor
class ShaderReplacement{
public:
	bool started = false;
	// if it has been enabled
	bool enabled = false;
	// tracks the last known dashboard open state for change detection
	bool lastDashboardOpenState = false;
	// tracks the last known connected headset for change detection
	Config::HeadsetType lastConnectedHeadset = Config::HeadsetType::None;
	// create hooks for shader loading
	void Initialize();
	// reload steamvr shaders
	void ReloadShaders();
	// thread to watch shaders
	void WatchShadersThread();
	// watch settings for changes that require a reload
	void CheckSettingsThread();
};