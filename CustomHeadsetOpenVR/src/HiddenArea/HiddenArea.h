#pragma once

#include "openvr_driver.h"
#include <vector>

struct HiddenAreaMeshConfig;

namespace HiddenArea {
	std::vector<vr::HmdVector2_t> CreateHiddenAreaMesh(const vr::EVREye eye, const vr::EHiddenAreaMeshType type, const HiddenAreaMeshConfig& config);
};
