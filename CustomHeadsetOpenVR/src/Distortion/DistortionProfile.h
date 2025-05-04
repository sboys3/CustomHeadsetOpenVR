#pragma once
#include "openvr_driver.h"

enum ColorChannel{
	ColorChannelRed,
	ColorChannelGreen,
	ColorChannelBlue,
};

struct Point2D{
	float x;
	float y;
};

class DistortionProfile{
public:
	// the resolution from -1 to 1 in the output coordinates
	float resolution;
	// called before the other functions are called
	virtual void Initialize(){};
	// fU and fV are normalized to be within [-1, 1] within the smallest box that fit on the screen
	// this means they can be largest than 1 in the larger dimension of the screen
	// that is not how this function is normally called in the openvr apis 
	virtual Point2D ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) = 0;
	// returns the raw projection details
	// the values are tangents of the half-angle from center axis
	// the top and bottom seemed to be reversed in the official documentation so the order is different here to correct that
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) = 0;
};