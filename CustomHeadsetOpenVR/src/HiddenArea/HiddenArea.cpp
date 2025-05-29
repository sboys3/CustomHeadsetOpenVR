#include "HiddenArea.h"
#include "../Config/Config.h"
#include "../Driver/DriverLog.h"

#include <algorithm>
#include <array>
#include <cmath>

constexpr double kPi { 3.141592653589793 };

static constexpr vr::HmdVector2_t makeHmdVec2(double x, double y) {
	return { static_cast<float>(x), static_cast<float>(y) };
}

std::vector<vr::HmdVector2_t> HiddenArea::CreateHiddenAreaMesh(const vr::EVREye eye, const vr::EHiddenAreaMeshType meshType, const HiddenAreaMeshConfig& config)
{
	if (meshType != vr::k_eHiddenAreaMesh_Standard && meshType != vr::k_eHiddenAreaMesh_Inverse && meshType != vr::k_eHiddenAreaMesh_LineLoop) {
		DriverLog("CreateHiddenAreaMesh: Invalid hidden area mesh type %d", static_cast<int>(meshType));
		return {};
	}

	// Helper struct
	struct AreaCorner{
		double x = 0.0;
		double y = 0.0;
		double radius = 0.0;
		double centerX = 0.0;
		double centerY = 0.0;

		AreaCorner() = delete;
		AreaCorner(double x_, double y_, double radius_)
			: x{x_}, y{y_}, radius(std::clamp(radius_, 0.001, 0.499)) // Not exactly 0.0 or 0.5 to avoid degenerate edges
		{
			centerX = x + std::copysign(radius, 0.5 - x);
			centerY = y + std::copysign(radius, 0.5 - y);
		}
	};

	const bool isRight = (eye == vr::Eye_Right);
	const bool isTestMode = config.testMode;
	const bool isLineLoop = (meshType == vr::k_eHiddenAreaMesh_LineLoop); // NB: I'm not 100% confident that the "Inverse" mesh isn't a line loop.
	const bool isCornerMesh = (meshType == vr::k_eHiddenAreaMesh_Standard && !isTestMode) || (meshType == vr::k_eHiddenAreaMesh_Inverse && isTestMode);
	const int segments = 2 + std::clamp(config.detailLevel, 1, 32);

	// Coordinates are a bit unusual since the display is rotated.
	const std::array<const AreaCorner, 4> corners {
		AreaCorner{ 1.0, 1.0, (isRight ? config.radiusTopOuter : config.radiusTopInner) },       // (1,1): NE Top Right
		AreaCorner{ 0.0, 1.0, (isRight ? config.radiusTopInner : config.radiusTopOuter) },       // (0,1): NW Top Left
		AreaCorner{ 0.0, 0.0, (isRight ? config.radiusBottomInner : config.radiusBottomOuter) }, // (0,0): SW Bottom Left
		AreaCorner{ 1.0, 0.0, (isRight ? config.radiusBottomOuter : config.radiusBottomInner) }  // (1,0): SE Bottom Right
	};

	// Create the mesh
	std::vector<vr::HmdVector2_t> mesh;

	double baseAngle = 0.f;
	const double angleIncr = kPi/(2.0 * (segments - 1));
	for (const AreaCorner& corner : corners) {
		for (int i = (isLineLoop ? 0 : 1); i < segments; ++i) {
			const double angle = baseAngle + (angleIncr * i);

			mesh.push_back(makeHmdVec2(
				corner.centerX + corner.radius * std::cos(angle),
				corner.centerY + corner.radius * std::sin(angle)
			));

			if (!isLineLoop) { // Complete a triangle
				const double prevAngle = baseAngle + (angleIncr * (i - 1));
				mesh.push_back(makeHmdVec2(
					corner.centerX + corner.radius * std::cos(prevAngle),
					corner.centerY + corner.radius * std::sin(prevAngle)
				));

				if (isCornerMesh) {
					mesh.push_back(makeHmdVec2(corner.x, corner.y));
				} else {
					mesh.push_back({ 0.5f, 0.5f });
				}
			}
		}
		baseAngle += (0.5 * kPi);
	}

	if (!isLineLoop && !isCornerMesh) {
		// Fill in the area between corner sections
		for (size_t cInd = 0; cInd < 4; ++cInd) {
			const auto& c1 = corners.at(cInd);
			const auto& c2 = corners.at((cInd + 1) % 4);

			mesh.push_back({ 0.5f, 0.5f });
			if (c1.x == c2.x) {
				mesh.push_back(makeHmdVec2(c1.x, c1.centerY));
				mesh.push_back(makeHmdVec2(c2.x, c2.centerY));
			} else {
				mesh.push_back(makeHmdVec2(c1.centerX, c1.y));
				mesh.push_back(makeHmdVec2(c2.centerX, c2.y));
			}
		}
	} else if (isLineLoop && isTestMode) {
		// Loop the mesh around the outside corners to invert the mesh (only the hidden area will be visible).
		// This is a bit hacky and doesn't show up properly in the SteamVR VR view, but it does show in the HMD.
		mesh.push_back({ 1.f, 0.f });
		mesh.push_back({ 0.f, 0.f });
		mesh.push_back({ 0.f, 1.f });
		mesh.push_back({ 1.f, 1.f });
	}

	return mesh;
}
