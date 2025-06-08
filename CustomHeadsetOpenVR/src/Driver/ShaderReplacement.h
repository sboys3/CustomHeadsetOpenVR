#pragma once

// replaces shaders within vrcompositor
class ShaderReplacement{
public:
	bool started = false;
	// if it has been enabled
	bool enabled = false;
	// create hooks for shader loading
	void Initialize();
	// reload steamvr shaders
	void ReloadShaders();
	// thread to watch shaders
	void WatchShadersThread();
	// watch settings for changes that require a reload
	void CheckSettingsThread();
};