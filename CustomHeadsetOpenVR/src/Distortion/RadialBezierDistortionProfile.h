#pragma once
#include "DistortionProfile.h"
#include <vector>
#include "../Driver/DriverLog.h"


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
	std::vector<DistortionPoint> distortions = {{0.f, 0.f}, {47.5f, 100.f}};
	// additional percent distortions for the red channel to be done after the main distortion
	std::vector<DistortionPoint> distortionsRed = {{0.f, 0.5f}, {47.5f, 0.5f}};
	// additional percent distortions for the blue channel to be done after the main distortion
	std::vector<DistortionPoint> distortionsBlue = {{0.f, -0.7f}, {47.5f, -0.7f}};
	// amount to smooth the curve from 0 to 1
	double smoothAmount = 0.66;
private:
	// this is automatically calculated from the distortions
	// this is half the fov of the input image in degrees
	float halfFovX = 0.0f;
	float halfFovY = 0.0f;
	// recommended render resolution
	uint32_t recommendedRenderWidth = 0;
	uint32_t recommendedRenderHeight = 0;
	// radial maps computed from distortions, the index is the output radius and the values are the input radius
	// these are ready to quickly compute the uv distortions
	float* radialUVMapR = nullptr;
	float* radialUVMapG = nullptr;
	float* radialUVMapB = nullptr;
	// conversion from radius in output to to an index in the maps
	float radialMapConversion = 0;
	// number of entires in each radial map
	int radialMapSize = 512;
	// number of smoothed points to create between each distortion point
	int inBetweenPoints = 20;
	// sample points from the radial maps
	inline float SampleFromMap(float* map, float radius) const;
	// helper function to compute statistics on ppd
	float ComputePPD(std::vector<DistortionPoint> distortion, float degreeStart, float degreeEnd);
	// delete allocated resources
	void Cleanup();
public:
	virtual void Initialize() override;
	
	virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) override;
	
	virtual Point2D ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) override;
	
	virtual void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) override;
	
	virtual ~RadialBezierDistortionProfile();
};