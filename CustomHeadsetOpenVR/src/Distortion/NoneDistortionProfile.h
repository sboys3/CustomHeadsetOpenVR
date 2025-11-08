#pragma once
#include "DistortionProfile.h"

#include <cmath>

// A distortion profile that does nothing
class NoneDistortionProfile : public DistortionProfile{
public:
	// horizontal fov for the none distortion profile
	float noneDistortionFovHorizontal = 100;
	// vertical fov for the none distortion profile
	float noneDistortionFovVertical = 100;


	virtual void Initialize() override{};
	
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) override{
		constexpr float kPi{ 3.1415926535897932384626433832795028841971693993751058209749445f };
		
		if(fovZoom == 0){
			fovZoom = 1.0f;
		}
		float hFovHalf = noneDistortionFovHorizontal / fovZoom / 2.0f;
		float vFovHalf = noneDistortionFovVertical / fovZoom / 2.0f;
		
		hFovHalf = hFovHalf * kPi / 180.0f;
		vFovHalf = vFovHalf * kPi / 180.0f;
		
		*pfLeft = std::tan(-hFovHalf);
		*pfRight = std::tan(hFovHalf);
		*pfTop = std::tan(vFovHalf);
		*pfBottom = std::tan(-vFovHalf);
	};
	
	virtual Point2D ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) override{
		return {fU, fV};
	};
};