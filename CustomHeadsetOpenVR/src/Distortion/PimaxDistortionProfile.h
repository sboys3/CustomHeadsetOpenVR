#pragma once
#include "DistortionProfile.h"

#include <cstdint>

#ifdef PVR_EXISTS
#include <PVR.h>

class PimaxDistortionProfile : public DistortionProfile {
public:
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) override;

	virtual Point2D ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) override;

	virtual void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) override;
};
#endif
