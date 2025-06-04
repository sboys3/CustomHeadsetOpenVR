#include "openvr_driver.h"



// Manually inject into the compositor process
// IVRCompositorPluginProvider requires a display redirect to work but that prevents video output.
// if forProcessId is specified it will only inject if forProcessId is vrcompositor.exe
bool InjectCompositorPlugin(int forProcessId = 0); 


class CompositorPlugin : public vr::IVRCompositorPluginProvider{
	/** initializes the driver when used to load compositor plugins */
	virtual vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override;

	/** cleans up the driver right before it is unloaded */
	virtual void Cleanup() override;

	/** Returns the versions of interfaces used by this driver */
	virtual const char* const* GetInterfaceVersions() override;

	/** Requests a component interface of the driver for specific functionality. The driver should return NULL
	* if the requested interface or version is not supported. */
	virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
};

// fake display redirect to allow compositor plugin to load
// I have not found a way to use this without breaking display output
// this can be initialized in the device provider like vr::VRServerDriverHost()->TrackedDeviceAdded("FakeDisplayRedirect", vr::TrackedDeviceClass_DisplayRedirect, &fakeDisplayRedirect);
class FakeDisplayRedirect : public vr::ITrackedDeviceServerDriver{
public:
	vr::EVRInitError Activate(uint32_t unObjectId) override {
		return vr::VRInitError_None;
	};
	void EnterStandby() override {
		
	};
	void *GetComponent(const char* pchComponentNameAndVersion) override {
		return nullptr;
	};
	void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override {
		if (unResponseBufferSize >= 1){
			pchResponseBuffer[ 0 ] = 0;
		}
	};
	vr::DriverPose_t GetPose() override {
		return {};
	};
	void Deactivate() override {
		
	};
};
