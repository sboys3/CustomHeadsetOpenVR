#include "HiddenArea.h"
#include "../Config/Config.h"

#include <algorithm>
#include <array>
#include <cmath>

constexpr double kPi { 3.141592653589793 };

std::vector<vr::HmdVector2_t> HiddenArea::CreateLineLoopMesh(vr::EVREye eye, const HiddenAreaMeshConfig& config)
{
	struct AreaCorner{
		double x = 0.0;
		double y = 0.0;
		double radius = 0.0;
	};

	const int segments = 2 + std::clamp(config.detailLevel, 1, 32);
	const bool testMode = config.testMode;

	const double topOut = config.radiusTopOuter;
	const double topIn  = config.radiusTopInner;
	const double botIn  = config.radiusBottomInner;
	const double botOut = config.radiusBottomOuter;
	const bool isRight = (eye == vr::Eye_Right);
	const double radNE = std::clamp(isRight ? topOut : topIn, 0.0, 0.5);
	const double radNW = std::clamp(isRight ? topIn : topOut, 0.0, 0.5);
	const double radSW = std::clamp(isRight ? botIn : botOut, 0.0, 0.5);
	const double radSE = std::clamp(isRight ? botOut : botIn, 0.0, 0.5);

	// Coordinates are a bit unusual since the display is rotated.
	const std::array<const AreaCorner, 4> corners {
		AreaCorner{ 1.0 - radNE, 1.0 - radNE, radNE }, // (1,1): NE Top Right
		AreaCorner{ 0.0 + radNW, 1.0 - radNW, radNW }, // (0,1): NW Top Left
		AreaCorner{ 0.0 + radSW, 0.0 + radSW, radSW }, // (0,0): SW Bottom Left
		AreaCorner{ 1.0 - radSE, 0.0 + radSE, radSE }  // (1,0): SE Bottom Right
	};

	// Create the mesh
	std::vector<vr::HmdVector2_t> mesh;
	mesh.reserve(4 * segments + (testMode ? 4 : 0));

	double baseAngle = 0.f;
	const double angleIncr = kPi/(2.0 * (segments - 1));
	for (const AreaCorner& corner : corners) {
		for (int i = 0; i < segments; ++i) {
			const double angle = baseAngle + (i * angleIncr);
			mesh.push_back({
				static_cast<float>(corner.x + corner.radius * std::cos(angle)),
				static_cast<float>(corner.y + corner.radius * std::sin(angle))
			});
		}
		baseAngle += (0.5 * kPi);
	}

	if (testMode) {
		// Loop the mesh around the outside corners to invert the mesh (only the hidden area will be visible).
		// This is a bit hacky and doesn't show up properly in the SteamVR VR view, but it does show in the HMD.
		mesh.push_back({ 1.f, 0.f });
		mesh.push_back({ 0.f, 0.f });
		mesh.push_back({ 0.f, 1.f });
		mesh.push_back({ 1.f, 1.f });
	}

	return mesh;
}
