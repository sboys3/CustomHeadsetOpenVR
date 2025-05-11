#pragma once
#include "DistortionProfile.h"

#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445

// A distortion profile that does nothing
class NoneDistortionProfile : public DistortionProfile{
public:
	// horizontal fov for the none distortion profile
	float noneDistortionFovHorizontal = 100;
	// vertical fov for the none distortion profile
	float noneDistortionFovVertical = 100;


	virtual void Initialize() override{};
	
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) override{
		float hFovHalf = noneDistortionFovHorizontal / 2.0f;
		float vFovHalf = noneDistortionFovVertical / 2.0f;
		
		hFovHalf = hFovHalf * (float)M_PI / 180.0f;
		vFovHalf = vFovHalf * (float)M_PI / 180.0f;
		
		*pfLeft = (float)tan(-hFovHalf);
		*pfRight = (float)tan(hFovHalf);
		*pfTop = (float)tan(vFovHalf);
		*pfBottom = (float)tan(-vFovHalf);
	};
	
	virtual Point2D ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) override{
		return {fU, fV};
	};
};