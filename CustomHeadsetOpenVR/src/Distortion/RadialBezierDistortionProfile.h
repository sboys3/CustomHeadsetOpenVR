#pragma once
#include "DistortionProfile.h"
#include <vector>
#include "../Driver/DriverLog.h"


#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445


class RadialBezierDistortionProfile : public DistortionProfile{
public:
	class DistortionPoint{
	public:
		// location in degrees in the input image
		float degree;
		// location on the display from 0 to 100
		float position;
	};
	// radial distortions from the input image in degrees to the output image in percent
	// this will be used as the values for the green channel
	std::vector<DistortionPoint> distortions ={{0, 0}, {47.5, 100}};
	// additional percent distortions for the red channel to be done after the main distortion
	std::vector<DistortionPoint> distortionsRed ={{0, 0.5}, {47.5, 0.5}};
	// additional percent distortions for the blue channel to be done after the main distortion
	std::vector<DistortionPoint> distortionsBlue ={{0, -0.42}, {47.5, -0.42}};
	
private:
	// this is automatically calculated from the distortions
	// this is the fov that is given by circle at radius 1
	float halfFov = 0.0f;
	// radial maps computed from distortions the index is the output image and the values are the input
	// these are ready to quickly compute the uv distortions
	float* radialUVMapR = nullptr;
	float* radialUVMapG = nullptr;
	float* radialUVMapB = nullptr;
	// conversion from radius in output to to an index in the maps
	float radialMapConversion = 0;
	int radialMapSize = 512;
	int inBetweenPoints = 20;
	inline float SampleFromMap(float* map, float radius);
	float ComputePPD(std::vector<DistortionPoint> distortion, float degreeStart, float degreeEnd);
	void Cleanup();
public:
	virtual void Initialize() override;
	
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) override;
	
	virtual Point2D ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) override;
	
	virtual ~RadialBezierDistortionProfile();
};