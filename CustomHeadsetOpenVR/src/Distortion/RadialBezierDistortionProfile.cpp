#include "RadialBezierDistortionProfile.h"

#include <cmath>

constexpr float kPi{ 3.1415926535897932384626433832795028841971693993751058209749445f };

using DistortionPoint = RadialBezierDistortionProfile::DistortionPoint;

// calculates a point on a cubic Bezier curve given a parameter t and a set of control points.
static DistortionPoint BezierPoint(float t, const std::vector<DistortionPoint>& controlPoints){
	float tSquared = t * t;
	float tCubed = t * t * t;
	float oneMinusT = 1 - t;
	float oneMinusTSquared = oneMinusT * oneMinusT;
	float oneMinusTCubed = oneMinusT * oneMinusT * oneMinusT;

	float pointX = (
		oneMinusTCubed * controlPoints[0].degree +
		3.f * oneMinusTSquared * t * controlPoints[1].degree +
		3.f * oneMinusT * tSquared * controlPoints[2].degree +
		tCubed * controlPoints[3].degree
	);
	float pointY = (
		oneMinusTCubed * controlPoints[0].position +
		3.f * oneMinusTSquared * t * controlPoints[1].position +
		3.f * oneMinusT * tSquared * controlPoints[2].position +
		tCubed * controlPoints[3].position
	);
	
	return DistortionPoint{pointX, pointY};
}

// SmoothPoints takes a list of points and returns a new list of points with additional points inserted between each pair of points using bezier curves.
static std::vector<DistortionPoint> SmoothPoints(const std::vector<DistortionPoint>& points, int innerPointCounts, float smoothAmount){
	// smoothAmount is how far out to move the center bezier points from the existing points
	// larger values will make the curve more "smooth" and less "sharp" at the existing points
	std::vector<DistortionPoint> outPoints;
	for(size_t i = 0; i < points.size() - 1; i++){
		// the new points will be inserted between existing points
		DistortionPoint prevPoint = points[i];
		DistortionPoint nextPoint = points[i + 1];
		DistortionPoint prevPrevPoint = i <= 0 ? points[i] : points[i - 1];
		DistortionPoint nextNextPoint = i >= points.size() - 2 ? points[i + 1] : points[i + 2];
		// find slope for prev and next point based on the points that surround them
		float fallbackSlope = (nextPoint.position - prevPoint.position) / (nextPoint.degree - prevPoint.degree);
		float prevSlope = i <= 0 ? fallbackSlope : (nextPoint.position - prevPrevPoint.position) / (nextPoint.degree - prevPrevPoint.degree);
		float nextSlope = (i >= points.size() - 2) ? fallbackSlope : (nextNextPoint.position - prevPoint.position) / (nextNextPoint.degree - prevPoint.degree);
		// extrapolate center points based on the slopes
		float centerDistance = (nextPoint.degree - prevPoint.degree) * smoothAmount;
		float centerFromPrev = centerDistance * prevSlope + prevPoint.position;
		float centerFromNext = -centerDistance * nextSlope + nextPoint.position;
		
		// create a bezier curve with the extrapolated center points and the existing points as anchors
		std::vector<DistortionPoint> controlPoints ={
			prevPoint,
			{prevPoint.degree + centerDistance, centerFromPrev},
			{nextPoint.degree - centerDistance, centerFromNext},
			nextPoint
		};
		
		outPoints.push_back(prevPoint);
		// generate inner points based on the bezier curve
		for(int j = 0; j < innerPointCounts; j++){
			outPoints.push_back(BezierPoint((j + 1) / static_cast<float>(innerPointCounts + 1), controlPoints));
		}
	}
	outPoints.push_back(points[points.size() - 1]);
	return outPoints;
}

// linear interpolation between two values based on a t value between 0 and 1
static constexpr float lerp(float a, float b, float t){
	return a + t * (b - a);
}

// sample a value from the points based on the degree
static float SampleFromPoints(const std::vector<DistortionPoint>& points, float degree){
	// find the two points that the degree is between
	for(size_t i = 0; i < points.size() - 1; i++){
		if(degree >= points[i].degree && degree <= points[i + 1].degree){
			// interpolate between the two points
			float t = (degree - points[i].degree) / (points[i + 1].degree - points[i].degree);
			return points[i].position + t * (points[i + 1].position - points[i].position);
		}
	}
	// if the degree is outside the range of the points, return the closest point
	if(degree < points[0].degree){
		return points[0].position;
	}else{
		// interpolate between the last two points
		auto i = points.size() - 2;
		float t = (degree - points[i].degree) / (points[i + 1].degree - points[i].degree);
		return lerp(points[i].position, points[i + 1].position, t);
	}
}

// inverse of SampleFromPoints, returns the degree for a given position
static float SampleFromPointsInverse(const std::vector<DistortionPoint>& points, float position){
	// find the two points that the position is between
	for(size_t i = 0; i < points.size() - 1; i++){
		if(position >= points[i].position && position <= points[i + 1].position){
			// interpolate between the two points
			float t = (position - points[i].position) / (points[i + 1].position - points[i].position);
			return points[i].degree + t * (points[i + 1].degree - points[i].degree);
		}
	}
	// if the position is outside the range of the points, return the closest point
	if(position < points[0].position){
		return points[0].degree;
	}else{
		// interpolate between the last two points
		auto i = points.size() - 2;
		float t = (position - points[i].position) / (points[i + 1].position - points[i].position);
		return lerp(points[i].degree, points[i + 1].degree, t);
	}
}


// sample from float map with linear interpolation
inline float RadialBezierDistortionProfile::SampleFromMap(float* map, float radius) const{
	float indexFloat = radius * radialMapConversion;
	int index = (int)(indexFloat);
	if(index < 0){
		index = 0;
	}else if(index >= radialMapSize - 1){
		index = radialMapSize - 2;
	}
	return lerp(map[index], map[index + 1], indexFloat - index);
}

// compute ppd in range
float RadialBezierDistortionProfile::ComputePPD(std::vector<DistortionPoint> distortion, float degreeStart, float degreeEnd){
	// compute ppd for the given range of degrees
	return (SampleFromPoints(distortion, degreeEnd) - SampleFromPoints(distortion, degreeStart)) / (degreeEnd - degreeStart) / 100.0f * resolution / 2.0f;
}

void RadialBezierDistortionProfile::Initialize(){
	Cleanup();
		// smooth the points
	std::vector<DistortionPoint> distortionsSmoothGreen = SmoothPoints(distortions, inBetweenPoints, (float)smoothAmount / 2.0f);
	std::vector<DistortionPoint> distortionsRedPercent = SmoothPoints(distortionsRed, inBetweenPoints, (float)smoothAmount / 2.0f);
	std::vector<DistortionPoint> distortionsBluePercent = SmoothPoints(distortionsBlue, inBetweenPoints, (float)smoothAmount / 2.0f);
	
	std::vector<DistortionPoint> distortionsSmoothRed = distortionsSmoothGreen;
	std::vector<DistortionPoint> distortionsSmoothBlue = distortionsSmoothGreen;
	// correct for chromatic aberration
	for(size_t i = 0; i < distortionsSmoothGreen.size(); i++){
		distortionsSmoothRed[i].position *= SampleFromPoints(distortionsRedPercent, distortionsSmoothRed[i].degree) / 100.0f + 1.0f;
		distortionsSmoothBlue[i].position *= SampleFromPoints(distortionsBluePercent, distortionsSmoothBlue[i].degree) / 100.0f + 1.0f;
		// halfFov = std::max(halfFov, distortionsSmoothGreen[i].degree);
	}
	
	// zoom fov
	for(size_t i = 0; i < distortionsSmoothGreen.size(); i++){
		distortionsSmoothRed[i].degree /= fovZoom;
		distortionsSmoothGreen[i].degree /= fovZoom;
		distortionsSmoothBlue[i].degree /= fovZoom;
	}
	
	
	DriverLog("== Distortion Statistics ==");
	DriverLog("PPD at 0°: %f\n", ComputePPD(distortionsSmoothGreen, 0, 1));
	DriverLog("PPD at 10°: %f\n", ComputePPD(distortionsSmoothGreen, 10, 11));
	DriverLog("PPD at 20°: %f\n", ComputePPD(distortionsSmoothGreen, 20, 21));
	DriverLog("PPD at 30°: %f\n", ComputePPD(distortionsSmoothGreen, 30, 31));
	DriverLog("PPD at 40°: %f\n", ComputePPD(distortionsSmoothGreen, 40, 41));
	
	DriverLog("PPD average 0° to 10°: %f\n", ComputePPD(distortionsSmoothGreen, 0, 10));
	DriverLog("PPD average 0° to 20°: %f\n", ComputePPD(distortionsSmoothGreen, 0, 20));
	
	DriverLog("Display resolution: %ix%i\n", (int)resolutionX, (int)resolutionY);
	
	// find maximum degrees that fit all color channels on the screen
	halfFovX = SampleFromPointsInverse(distortionsSmoothRed, resolutionX / resolution * 100.0f);
	halfFovX = std::min(halfFovX, SampleFromPointsInverse(distortionsSmoothGreen, resolutionX / resolution * 100.0f));
	halfFovX = std::min(halfFovX, SampleFromPointsInverse(distortionsSmoothBlue, resolutionX / resolution * 100.0f));
	halfFovY = SampleFromPointsInverse(distortionsSmoothRed, resolutionY / resolution * 100.0f);
	halfFovY = std::min(halfFovY, SampleFromPointsInverse(distortionsSmoothGreen, resolutionY / resolution * 100.0f));
	halfFovY = std::min(halfFovY, SampleFromPointsInverse(distortionsSmoothBlue, resolutionY / resolution * 100.0f));
	
	// limit to max fov
	halfFovX = std::min(halfFovX, maxFovX / 2.0f);
	halfFovY = std::min(halfFovY, maxFovY / 2.0f);
	
	// halfFovY = halfFovX;
	DriverLog("FOV: %fx%f\n", halfFovX * 2, halfFovY * 2);
	// halfFovX = 20;
	// halfFovY = 20;
	
	
	// convert to input coordinates and flip the point values to sample from output to input
	for (size_t i = 0; i < distortionsSmoothGreen.size(); i++){
		// use tangent to convert from degrees into input screen space
		distortionsSmoothRed[i].degree   = std::tan(distortionsSmoothRed[i].degree   * kPi / 180.0f);
		distortionsSmoothGreen[i].degree = std::tan(distortionsSmoothGreen[i].degree * kPi / 180.0f);
		distortionsSmoothBlue[i].degree  = std::tan(distortionsSmoothBlue[i].degree  * kPi / 180.0f);
	}
	
	// calculate tangents for the edges of the input screen
	float edgeTanX = std::tan(halfFovX * kPi / 180.0f);
	float edgeTanY = std::tan(halfFovY * kPi / 180.0f);
	
	// calculate the maximum output percentage for the edge of the output
	float maxOutputPercentageX = SampleFromPoints(distortionsSmoothGreen, edgeTanX);
	float maxOutputPercentageY = SampleFromPoints(distortionsSmoothGreen, edgeTanY);
	// calculate the maximum ratio between input and output image pixels
	float maxInputOutputRatioX = 0.0f;
	float maxInputOutputRatioY = 0.0f;
	for(size_t i = 0; i < distortionsSmoothGreen.size() - 1; i++){
		DistortionPoint prevPoint = distortionsSmoothGreen[i];
		DistortionPoint nextPoint = distortionsSmoothGreen[i + 1];
		float inputOutputRatioX = (nextPoint.position - prevPoint.position) / maxOutputPercentageX / ((nextPoint.degree - prevPoint.degree) / edgeTanX);
		maxInputOutputRatioX = std::max(maxInputOutputRatioX, inputOutputRatioX);
		float inputOutputRatioY = (nextPoint.position - prevPoint.position) / maxOutputPercentageY / ((nextPoint.degree - prevPoint.degree) / edgeTanY);
		maxInputOutputRatioY = std::max(maxInputOutputRatioY, inputOutputRatioY);
		// DriverLog("distortion ratio: %f", inputOutputRatio);
	}
	
	// calculate the resolution of the output screen that is actually used based on the maximum output percentage for the given fov
	float usedOutputScreenResolutionX = maxOutputPercentageX / 100.0f * resolution;
	float usedOutputScreenResolutionY = maxOutputPercentageY / 100.0f * resolution;
	DriverLog("Used output screen resolution: %fx%f", usedOutputScreenResolutionX, usedOutputScreenResolutionY);
	// calculate the required input resoltions required to get 1:1 distortion in the most distorted spot
	float desiredInputResolutionX = maxInputOutputRatioX * usedOutputScreenResolutionX;
	float desiredInputResolutionY = maxInputOutputRatioY * usedOutputScreenResolutionY;
	recommendedRenderWidth = (uint32_t)(desiredInputResolutionX);
	recommendedRenderHeight = (uint32_t)(desiredInputResolutionY);
	
	DriverLog("Max distortion ratio X: %f", maxInputOutputRatioX);
	DriverLog("Max distortion ratio Y: %f", maxInputOutputRatioY);
	// steamvr lists percentage as total number of pixels, not a single dimension
	DriverLog("Oversampling required for 1:1 distortion: %f%% %ix%i", ((desiredInputResolutionX * desiredInputResolutionY) / (resolutionX * resolutionY)) * 100.0f, (int)(desiredInputResolutionX), (int)(desiredInputResolutionY));
	
	// calculate aspect ratios of output and desired input resolution
	DriverLog("Display aspect ratio : %f\n", resolutionX / resolutionY);
	DriverLog("Used output aspect ratio : %f\n", usedOutputScreenResolutionX / usedOutputScreenResolutionY);
	DriverLog("1:1 input aspect ratio : %f\n", desiredInputResolutionX / desiredInputResolutionY);
	
	if(false){
		char* distortionPointLog = new char[distortionsSmoothGreen.size() * 40];
		int distortionPointLogSize = 0;
		for(size_t i = 0; i < distortionsSmoothGreen.size(); i++){
			distortionPointLogSize += sprintf(distortionPointLog + distortionPointLogSize, "[%f, %f] ", distortionsSmoothBlue[i].position, distortionsSmoothBlue[i].degree);
		}
		// DriverLog("distortion points: %s", distortionPointLog);
		delete[] distortionPointLog;
	}
	
	// create radial maps
	radialUVMapR = new float[radialMapSize];
	radialUVMapG = new float[radialMapSize];
	radialUVMapB = new float[radialMapSize];
	// calculate how much further the radial map needs to go to cover the entire screen area
	float distancePastOne = std::max(resolutionX, resolutionY) / resolution;
	radialMapConversion = (float)radialMapSize / distancePastOne;
	for(int i = 0; i < radialMapSize; i++){
		float outputRadius = i / radialMapConversion * 100;
		radialUVMapR[i] = SampleFromPointsInverse(distortionsSmoothRed, outputRadius);
		radialUVMapG[i] = SampleFromPointsInverse(distortionsSmoothGreen, outputRadius);
		radialUVMapB[i] = SampleFromPointsInverse(distortionsSmoothBlue, outputRadius);
	}
	
	if(false){
		char* radialMapLog = new char[radialMapSize * 20];
		int radialMapLogSize = 0;
		for(int i = 200; i < radialMapSize; i++){
			radialMapLogSize += sprintf(radialMapLog + radialMapLogSize, "%f ", radialUVMapB[i]);
		}
		DriverLog("distortion radial map: %s", radialMapLog);
		delete[] radialMapLog;
	}
	
}

void RadialBezierDistortionProfile::GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop){
	// DriverLog("GetProjectionRaw returning an fov of %f", halfFov * 2.0f);
	const float hFovHalf = halfFovX * kPi / 180.0f; // Convert to radians
	const float vFovHalf = halfFovY * kPi / 180.0f; // Convert to radians
	
	*pfLeft = std::tan(-hFovHalf);
	*pfRight = std::tan(hFovHalf);
	*pfTop = std::tan(vFovHalf);
	*pfBottom = std::tan(-vFovHalf);
}

Point2D RadialBezierDistortionProfile::ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV){
	
	// convert to radius and unit vector
	float radius = std::sqrt(fU * fU + fV * fV);
	float unitU = fU / radius;
	float unitV = fV / radius;
	// fix NaNs
	if(!std::isfinite(unitU)){
		unitU = 0;
	}
	if(!std::isfinite(unitV)){
		unitV = 0;
	}
	
	// sample distortion map for the given radius and color channel
	switch (colorChannel){
		case ColorChannelRed:
			radius = SampleFromMap(radialUVMapR, radius);
			break;
		case ColorChannelGreen:
			radius = SampleFromMap(radialUVMapG, radius);
			break;
		case ColorChannelBlue:
			radius = SampleFromMap(radialUVMapB, radius);
			break;
	}
	
	// convert back to points and return
	Point2D distortion;
	distortion.x = unitU * radius;
	distortion.y = unitV * radius;
	distortion.x /= std::tan(halfFovX * kPi / 180.0f);
	distortion.y /= std::tan(halfFovY * kPi / 180.0f);
	return distortion;
}

void RadialBezierDistortionProfile::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight){
	*pnWidth = recommendedRenderWidth;
	*pnHeight = recommendedRenderHeight;
}

void RadialBezierDistortionProfile::Cleanup(){
	if(radialUVMapR != nullptr){
		delete[] radialUVMapR;
		radialUVMapR = nullptr;
	}
	if(radialUVMapG != nullptr){
		delete[] radialUVMapG;
		radialUVMapG = nullptr;
	}
	if(radialUVMapB != nullptr){
		delete[] radialUVMapB;
		radialUVMapB = nullptr;
	}
}

RadialBezierDistortionProfile::~RadialBezierDistortionProfile(){
	Cleanup();
}