#pragma once

#include <cstdint>
#include <string>

// Decode a DXBC shader blob into HLSL interface declarations.
// Accepts raw DXBC binary (validated by "DXBC" magic). Parses RDEF for
// resources and constant buffers, ISGN/ISG1 for input signatures, and
// OSGN/OSG5/OSG1 for output signatures. Returns a string containing
// HLSL declarations: samplers, textures, UAVs, structured buffers,
// struct definitions, cbuffer blocks with packoffsets, and input/output
// signature structs. Returns empty string on any error.
std::string DecodeShaderVariables(const uint8_t *data, uint32_t size);
