#include "DistortionProfile.h"


class DistortionModifier{
public:
	float horizontalRotation = 0;
	float verticalRotation = 0;
	float axialRotation = 0;
	
	float fovLeft = 0;
	float fovRight = 0;
	float fovTop = 0;
	float fovBottom = 0;
private:
	float outputFovLeft = 0;
	float outputFovRight = 0;
	float outputFovTop = 0;
	float outputFovBottom = 0;
	
	
public:
	// convert a vector in the output space to the input space
	Point3D DistortVector(Point3D vector);
	// convert a point on the output display UV to a vector in the output space
	Point3D OutputPointToVector(Point2D point);
	// convert a vector in the input space to a point on the input display UV
	Point2D VectorToInputPoint(Point3D vector);
	
	vr::HmdMatrix34_t ModifyEyeMatrix(vr::HmdMatrix34_t matrix);
	Point2D ModifyDistortionPoint(Point2D point);
	
	DistortionModifier();
	~DistortionModifier();
};