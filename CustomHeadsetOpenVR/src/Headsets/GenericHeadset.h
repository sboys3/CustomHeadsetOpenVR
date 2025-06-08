#include "../Driver/DeviceShim.h"
#include "../Driver/DeviceProvider.h"
#include "../Driver/DriverLog.h"



class GenericHeadsetShim : public ShimDefinition{
public:
	CustomHeadsetDeviceProvider* deviceProvider = nullptr;
	
	virtual void PosTrackedDeviceActivate(uint32_t &unObjectId, vr::EVRInitError &returnValue) override;
};