#pragma once

#include "openvr_driver.h"
#include <vector>

struct HiddenAreaMeshConfig;

namespace HiddenArea {
	std::vector<vr::HmdVector2_t> CreateLineLoopMesh(vr::EVREye eye, const HiddenAreaMeshConfig& config);
};
