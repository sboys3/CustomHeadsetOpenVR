#include "ShaderDecode.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>

#include "../Driver/DriverLog.h"

// --- DXBC Format Constants ---

enum ShaderVarClass : uint16_t {
	SVC_SCALAR = 0, SVC_VECTOR = 1, SVC_MATRIX_ROWS = 2, SVC_MATRIX_COLS = 3,
	SVC_OBJECT = 4, SVC_STRUCT = 5
};

enum ShaderVarType : uint16_t {
	SVT_VOID = 0, SVT_BOOL = 1, SVT_INT = 2, SVT_FLOAT = 3,
	SVT_STRING = 4, SVT_TEXTURE = 5, SVT_TEXTURE1D = 6, SVT_SAMPLER = 7,
	SVT_PIXELSHADER = 9, SVT_VERTEXSHADER = 10,
	SVT_PIXELFRAGMENT = 11, SVT_VERTEXFRAGMENT = 12,
	SVT_UINT = 13, SVT_UINT8 = 14, SVT_GEOMETRYSHADER = 15,
	SVT_TEXTURE2D = 18, SVT_TEXTURE3D = 19, SVT_TEXTURECUBE = 20,
	SVT_BUFFER = 21, SVT_TEXTURE1DARRAY = 22, SVT_TEXTURE2DARRAY = 23,
	SVT_TEXTURE2DMS = 25, SVT_TEXTURE2DMSARRAY = 26, SVT_TEXTURECUBEARRAY = 27,
	SVT_DOUBLE = 32, SVT_RWTEXTURE1D = 33, SVT_RWTEXTURE2D = 35,
	SVT_RWTEXTURE3D = 37, SVT_RWBUFFER = 38, SVT_UINT64 = 40,
};

enum ShaderInputType : uint32_t {
	SIT_CBUFFER = 0, SIT_TBUFFER = 1, SIT_TEXTURE = 2, SIT_SAMPLER = 3,
	SIT_UAV_RWTYPED = 4, SIT_STRUCTURED = 5, SIT_BYTE_ADDRESS = 7,
};

enum SRVDimension : uint32_t {
	SRV_UNKNOWN = 0, SRV_BUFFER = 1, SRV_TEXTURE1D = 2, SRV_TEXTURE2D = 3,
	SRV_TEXTURE2DMS = 4, SRV_TEXTURE3D = 5, SRV_TEXTURECUBE = 6,
	SRV_TEXTURE1DARRAY = 7, SRV_TEXTURE2DARRAY = 8, SRV_TEXTURE2DMSARRAY = 9,
	SRV_TEXTURECUBEARRAY = 10,
};

enum CompType : uint32_t {
	CT_UNKNOWN = 0, CT_UINT = 1, CT_SINT = 2, CT_FLOAT = 3,
	CT_UINT16 = 4, CT_SINT16 = 5, CT_FLOAT16 = 6,
	CT_UINT64 = 7, CT_SINT64 = 8, CT_FLOAT64 = 9,
};

// --- Data Structures ---

struct ResourceDecl {
	std::string name;
	uint32_t bindPoint;
	uint32_t bindCount;
	ShaderInputType inputType;
	SRVDimension dimension;
	uint32_t flags;
};

struct CBField {
	std::string name;
	uint32_t byteOffset;
	ShaderVarClass cls;
	ShaderVarType type;
	uint16_t rows;
	uint16_t cols;
	uint16_t elements;
};

struct StructMember {
	std::string name;
	ShaderVarClass cls;
	ShaderVarType type;
	uint16_t rows;
	uint16_t cols;
	uint16_t elements;
	uint32_t byteOffset;
	uint32_t typeOff;
};

struct StructDef {
	std::string name;
	std::vector<StructMember> members;
};

struct ConstantBuffer {
	std::string name;
	uint32_t bindPoint;
	uint32_t bufferSize;
	std::vector<CBField> fields;
};

struct SigParam {
	std::string semanticName;
	uint32_t semanticIndex;
	std::string typeName;
	uint32_t registerIdx;
};

struct ShaderInfo {
	std::vector<ResourceDecl> resources;
	std::vector<ConstantBuffer> cbufs;
	std::vector<SigParam> inputParams;
	std::vector<SigParam> outputParams;
	std::vector<StructDef> structs;
};

// --- Internal Structures ---

struct DXBCPart {
	uint8_t fourcc[4];
	uint32_t size;
	const uint8_t *data;
};

// --- Safe Binary Read Helpers ---

// Verify that [off, off+len) fits within [0, sz). Guards against integer
// overflow by rejecting any operand larger than sz before adding.
static bool checkBounds(const uint8_t *, uint32_t sz, uint32_t off, uint32_t len) {
	if (off > sz || len > sz) return false;
	return off + len <= sz;
}

// Read a little-endian uint32 from data[off]. Returns 0 on out-of-bounds access.
static uint32_t readU32(const uint8_t *data, uint32_t sz, uint32_t off) {
	if (!checkBounds(data, sz, off, 4)) return 0;
	uint32_t v;
	std::memcpy(&v, data + off, 4);
	return v;
}

// Read a null-terminated ASCII string from offset. Only accepts printable chars
// (0x20-0x7E). Clamps result to 1024 characters. DXBC string offsets are file-relative.
static std::string readString(const uint8_t *data, uint32_t sz, uint32_t off) {
	if (off >= sz) return "";
	std::string s;
	while (off < sz && data[off] >= 0x20 && data[off] < 0x7F) {
		s += (char)data[off++];
	}
	if (s.length() > 1024) s = s.substr(0, 1024);
	return s;
}

// --- Type Resolution ---

// Return true if the type is a numeric scalar (void/bool/int/float/uint/double/uint64).
// These types are valid for scalars and vectors; non-numeric types get coerced to float.
static bool isNumericType(ShaderVarType t) {
	return t == SVT_VOID || t == SVT_BOOL || t == SVT_INT || t == SVT_FLOAT ||
		t == SVT_UINT || t == SVT_UINT8 || t == SVT_DOUBLE || t == SVT_UINT64;
}

// Coerce non-numeric types (e.g. SVT_TEXTURE3D) to float for scalar/vector contexts
// where the DXBC format reuses type codes for dimensional information.
static ShaderVarType normalizeType(ShaderVarClass cls, ShaderVarType type) {
	if ((cls == SVC_SCALAR || cls == SVC_VECTOR) && !isNumericType(type)) {
		if (type == SVT_TEXTURE3D) return SVT_UINT;
		return SVT_FLOAT;
	}
	return type;
}

// Return the byte size of a single component: 8 bytes for double/uint64, 4 for all others.
static uint32_t getTypeComponentSize(ShaderVarType type) {
	return (type == SVT_DOUBLE || type == SVT_UINT64) ? 8 : 4;
}

// Compute total byte size of a typed field (scalar, vector, or matrix).
// Normalizes the type first so non-numeric types resolve to float-sized components.
static uint32_t getFieldByteSize(ShaderVarClass cls, ShaderVarType type, uint16_t rows, uint16_t cols) {
	type = normalizeType(cls, type);
	uint32_t compSize = getTypeComponentSize(type);
	if (cls == SVC_MATRIX_COLS || cls == SVC_MATRIX_ROWS) return rows * cols * compSize;
	if (cls == SVC_VECTOR) {
		uint32_t dim = cols > 0 ? cols : (rows > 0 ? rows : 1);
		return dim * compSize;
	}
	return compSize;
}

// Map a ShaderVarType enum value to its HLSL scalar type name.
static const char* typeToHLSLBase(ShaderVarType t) {
	switch (t) {
		case SVT_VOID:   return "void";
		case SVT_BOOL:   return "bool";
		case SVT_INT:    return "int";
		case SVT_FLOAT:  return "float";
		case SVT_UINT:   return "uint";
		case SVT_DOUBLE: return "double";
		case SVT_UINT64: return "uint64_t";
		case SVT_UINT8:  return "uint8_t";
		default:         return "float";
	}
}

// Combine class (scalar/vector/matrix), base type, and dimensions into an HLSL type
// string such as "float4", "int3x4", "double", etc.
static std::string typeToHLSL(ShaderVarClass cls, ShaderVarType type, uint16_t rows, uint16_t cols) {
	type = normalizeType(cls, type);
	const char* base = typeToHLSLBase(type);
	if (cls == SVC_SCALAR) return base;
	if (cls == SVC_MATRIX_COLS || cls == SVC_MATRIX_ROWS)
		return std::string(base) + std::to_string(rows) + "x" + std::to_string(cols);
	if (cls == SVC_VECTOR) {
		uint32_t dim = cols > 0 ? cols : (rows > 0 ? rows : 1);
		if (dim == 1) return base;
		return std::string(base) + std::to_string(dim);
	}
	return base;
}

	// Map an SRV dimension enum to an HLSL TextureN<...> type string.
	// Defaults to Texture2D<float4> for unknown dimensions.
	static std::string dimToTextureType(SRVDimension dim) {
		switch (dim) {
			case SRV_TEXTURE1D:      return "Texture1D<float4>";
			case SRV_TEXTURE2D:      return "Texture2D<float4>";
			case SRV_TEXTURE3D:      return "Texture3D<float4>";
			case SRV_TEXTURECUBE:    return "TextureCube<float4>";
			case SRV_TEXTURE1DARRAY: return "Texture1DArray<float4>";
			case SRV_TEXTURE2DARRAY: return "Texture2DArray<float4>";
			case SRV_TEXTURECUBEARRAY: return "TextureCubeArray<float4>";
			default:                 return "Texture2D<float4>";
		}
	}

	// Map UAV dimension values from RDEF to an HLSL RWTextureN<...> type string.
	// The dimension enum in RDEF differs from both SRV and D3D11_UAV_DIMENSION; this
	// mapping is derived empirically from compiled shaders.
	static std::string dimToRWTextureType(uint32_t dim) {
		switch (dim) {
			case 1: return "RWBuffer<float4>";
			case 2: return "RWTexture1D<float4>";
			case 3: return "RWTexture2D<float4>";
			case 4: return "RWTexture2D<float4>";
			case 5: return "RWTexture1DArray<float4>";
			case 6: return "RWTexture3D<float4>";
			case 7: return "RWTextureCube<float4>";
			case 8: return "RWTexture2DArray<float4>";
			default: return "RWTexture2D<float4>";
		}
	}

	// Return the structured buffer HLSL type. For dim 0/1 returns "StructuredBuffer";
	// could be extended for AppendStructuredBuffer/ConsumeStructuredBuffer variants.
	static std::string dimToStructuredType(uint32_t dim) {
		// Structured buffers use dim=0 or 1 in RDEF
		if (dim == 0 || dim == 1) return "StructuredBuffer";
		return "StructuredBuffer";
	}

static std::string compTypeToHLSL(CompType ct) {
	// Map a signature component type (from ISGN/OSGN parameter descriptors) to an
	// HLSL scalar type name. Used for vertex/pixel shader input/output types.
	switch (ct) {
		case CT_FLOAT:   return "float";
		case CT_SINT:    return "int";
		case CT_UINT:    return "uint";
		case CT_FLOAT16: return "half";
		case CT_FLOAT64: return "double";
		default:         return "float";
	}
}

// --- DXBC Container ---

// Parse the DXBC container header and part table. Returns a list of parts with
// FourCC tag, payload size, and pointer to payload data. Validates magic "DXBC",
// part count (1-64), and bounds-checks every part offset.
static std::vector<DXBCPart> parseContainer(const uint8_t *data, uint32_t sz) {
	std::vector<DXBCPart> parts;
	if (sz < 32) return parts;
	if (std::memcmp(data, "DXBC", 4) != 0) return parts;

	uint32_t nParts = readU32(data, sz, 28);
	if (nParts == 0 || nParts > 64) return parts;

	for (uint32_t i = 0; i < nParts; i++) {
		uint32_t partOff = readU32(data, sz, 32 + i * 4);
		if (!checkBounds(data, sz, partOff, 8)) break;

		uint32_t partSz = readU32(data, sz, partOff + 4);
		if (!checkBounds(data, sz, partOff + 8, partSz)) break;

		DXBCPart p;
		std::memcpy(p.fourcc, data + partOff, 4);
		p.size = partSz;
		p.data = data + partOff + 8;
		parts.push_back(p);
	}
	return parts;
}

// Linear search for a part by FourCC tag (e.g. "RDEF", "ISGN"). Returns true and
// fills out with the matching part if found.
static bool findPart(const std::vector<DXBCPart> &parts, const char *tag, DXBCPart &out) {
	for (size_t i = 0; i < parts.size(); i++) {
		if (std::memcmp(parts[i].fourcc, tag, 4) == 0) {
			out = parts[i];
			return true;
		}
	}
	return false;
}

// --- Struct Parsing ---

// Recursively parse a type descriptor at typeOff into a StructDef. Nested structs are
// appended to 'collected' before the top-level struct. 'seenOffsets' prevents infinite
// recursion on circular references. Reads class/type from low dword, rows/cols from
// mid dword, elements/members from high dword of the type descriptor.
static void parseStructDesc(const uint8_t *data, uint32_t sz, uint32_t typeOff,
	std::vector<StructDef> &collected,
	std::vector<uint32_t> &seenOffsets,
	StructDef &out) {

	// Initialize output
	out.name = "";
	out.members.clear();

	if (!checkBounds(data, sz, typeOff, 16)) return;

	uint32_t classType = readU32(data, sz, typeOff + 0);
	uint32_t elemsMembers = readU32(data, sz, typeOff + 8);

	uint16_t cls = (uint16_t)(classType & 0xFFFF);
	uint16_t members = (uint16_t)(elemsMembers >> 16);

	if (checkBounds(data, sz, typeOff + 32, 4)) {
		uint32_t nameOff = readU32(data, sz, typeOff + 32);
		std::string n = readString(data, sz, nameOff);
		if (!n.empty()) out.name = n;
	}

	for (size_t i = 0; i < seenOffsets.size(); i++) {
		if (seenOffsets[i] == typeOff) return;
	}
	seenOffsets.push_back(typeOff);

	if (cls == SVC_STRUCT && members > 0) {
		uint32_t memberOff = readU32(data, sz, typeOff + 12);
		uint32_t memberTableSize = members * 12;
		if (!checkBounds(data, sz, memberOff, memberTableSize)) {
			seenOffsets.pop_back();
			return;
		}

		for (uint32_t k = 0; k < members; k++) {
			uint32_t mbase = memberOff + k * 12;
			StructMember mem;
			mem.name = readString(data, sz, readU32(data, sz, mbase + 0));
			uint32_t mtypeOff = readU32(data, sz, mbase + 4);
			mem.byteOffset = readU32(data, sz, mbase + 8);
			mem.typeOff = mtypeOff;
			mem.cls = SVC_SCALAR;
			mem.type = SVT_FLOAT;
			mem.rows = 1;
			mem.cols = 1;
			mem.elements = 0;

			if (checkBounds(data, sz, mtypeOff, 12)) {
				uint32_t mclassType = readU32(data, sz, mtypeOff + 0);
				uint32_t mrowsCols = readU32(data, sz, mtypeOff + 4);
				uint32_t melemsMembers = readU32(data, sz, mtypeOff + 8);
				mem.cls = (ShaderVarClass)(mclassType & 0xFFFF);
				mem.type = (ShaderVarType)(mclassType >> 16);
				mem.rows = (uint16_t)(mrowsCols & 0xFFFF);
				mem.cols = (uint16_t)(mrowsCols >> 16);
				mem.elements = (uint16_t)(melemsMembers & 0xFFFF);

				if (mem.rows > 64) mem.rows = 1;
				if (mem.cols > 64) mem.cols = 1;
				if (mem.elements > 1024) mem.elements = 0;

				if (mem.cls == SVC_STRUCT) {
					StructDef nested;
					parseStructDesc(data, sz, mtypeOff, collected, seenOffsets, nested);
					collected.push_back(std::move(nested));
				}
			}

			out.members.push_back(std::move(mem));
		}
	}

	seenOffsets.pop_back();
}

// --- RDEF Parser ---

// Parse the resource definition chunk (RDEF). Extracts two tables:
// 1. Resource table: textures, samplers, UAVs, structured buffers with bind points.
//    Entries with inputType==0 are cbuffer binding hints (name -> register slot).
// 2. Constant buffer table: each cbuffer with its fields, types, and offsets.
//    SM4 uses 24-byte variable descriptors; SM5 uses 40-byte.
static void parseRDEF(const uint8_t *data, uint32_t sz, ShaderInfo &info) {
	if (sz < 28) return;

	// RDEF header: counts, offsets, target shader model
	uint32_t cbCount = readU32(data, sz, 0);
	uint32_t cbOff = readU32(data, sz, 4);
	uint32_t resCount = readU32(data, sz, 8);
	uint32_t resOff = readU32(data, sz, 12);
	uint32_t targetVer = readU32(data, sz, 16);

	uint8_t majorVer = (uint8_t)((targetVer >> 8) & 0xFF);
	bool isSM5 = (majorVer >= 5);
	uint32_t varStride = isSM5 ? 40 : 24;

	std::map<std::string, uint32_t> cbBindMap;
	if (resCount > 0 && resCount <= 256) {
		uint32_t resTableSize = resCount * 32;
		if (!checkBounds(data, sz, resOff, resTableSize)) resCount = 0;
	}

	if (resCount > 0) {
		for (uint32_t i = 0; i < resCount; i++) {
			uint32_t base = resOff + i * 32;
			if (!checkBounds(data, sz, base, 32)) break;

			uint32_t nameOff = readU32(data, sz, base + 0);
			uint32_t inputType = readU32(data, sz, base + 4);
			uint32_t dimension = readU32(data, sz, base + 12);
			uint32_t bindPoint = readU32(data, sz, base + 20);
			uint32_t bindCount = readU32(data, sz, base + 24);
			uint32_t flags = readU32(data, sz, base + 28);

			std::string rname = readString(data, sz, nameOff);

			if (inputType == 0) {
				cbBindMap[rname] = bindPoint;
				continue;
			}

			if (bindCount == 0) bindCount = 1;
			if (bindPoint > 256) continue;

			// Resource types: 1=CB, 2=Texture, 3=Sampler, 4=UAV, 5=Structured, 7=ByteAddress

			ResourceDecl res;
			res.name = rname;
			res.bindPoint = bindPoint;
			res.bindCount = bindCount;
			res.inputType = (ShaderInputType)inputType;
			res.dimension = (SRVDimension)dimension;
			res.flags = flags;
			info.resources.push_back(res);
		}
	}

	if (cbCount > 0 && cbCount <= 64) {
		uint32_t cbTableSize = cbCount * 24;
		if (!checkBounds(data, sz, cbOff, cbTableSize)) cbCount = 0;
	}

	if (cbCount > 0) {
		for (uint32_t i = 0; i < cbCount; i++) {
			uint32_t base = cbOff + i * 24;
			if (!checkBounds(data, sz, base, 24)) break;

			uint32_t nameOff = readU32(data, sz, base + 0);
			uint32_t varCount = readU32(data, sz, base + 4);
			uint32_t varDescOff = readU32(data, sz, base + 8);
			uint32_t bufSize = readU32(data, sz, base + 12);

			ConstantBuffer cbuf;
			cbuf.name = readString(data, sz, nameOff);
			auto it = cbBindMap.find(cbuf.name);
			cbuf.bindPoint = (it != cbBindMap.end()) ? it->second : i;
			cbuf.bufferSize = bufSize;

			if (varCount > 0 && varCount <= 512) {
				uint32_t varTableSize = varCount * varStride;
				if (!checkBounds(data, sz, varDescOff, varTableSize)) varCount = 0;
			}

			if (varCount > 0) {
				for (uint32_t j = 0; j < varCount; j++) {
					uint32_t vbase = varDescOff + j * varStride;
					if (!checkBounds(data, sz, vbase, varStride)) break;

					uint32_t vNameOff = readU32(data, sz, vbase + 0);
					uint32_t vByteOff = readU32(data, sz, vbase + 4);
					uint32_t vTypeOff = readU32(data, sz, vbase + 16);

					CBField field;
					field.name = readString(data, sz, vNameOff);
					field.byteOffset = vByteOff;
					field.cls = SVC_SCALAR;
					field.type = SVT_FLOAT;
					field.rows = 1;
					field.cols = 1;
					field.elements = 0;

					if (checkBounds(data, sz, vTypeOff, 16)) {
						uint32_t classType = readU32(data, sz, vTypeOff + 0);
						uint32_t rowsCols = readU32(data, sz, vTypeOff + 4);
						uint32_t elemsMembers = readU32(data, sz, vTypeOff + 8);

						field.cls = (ShaderVarClass)(classType & 0xFFFF);
						field.type = (ShaderVarType)(classType >> 16);
						field.rows = (uint16_t)(rowsCols & 0xFFFF);
						field.cols = (uint16_t)(rowsCols >> 16);
						field.elements = (uint16_t)(elemsMembers & 0xFFFF);

						if (field.rows > 64) field.rows = 1;
						if (field.cols > 64) field.cols = 1;
						if (field.elements > 1024) field.elements = 0;

						if (field.cls == SVC_STRUCT) {
							std::vector<uint32_t> seen;
							std::vector<StructDef> collected;
							StructDef sd;
							parseStructDesc(data, sz, vTypeOff, collected, seen, sd);
							for (auto &nc : collected) info.structs.push_back(std::move(nc));
							info.structs.push_back(std::move(sd));
						}
					}

					cbuf.fields.push_back(field);
				}
			}

			info.cbufs.push_back(cbuf);
		}
	}
}

// --- Signature Parser ---

// Parse an input or output signature chunk (ISGN, ISG1, OSGN, OSG5, OSG1, PSG1).
// Determines record layout from the FourCC: ISGN/OSGN are 24-byte records,
// OSG5 is 28-byte, and ISG1/OSG1/PSG1 are 32-byte. Each record contains semantic
// name offset, semantic index, component type, register index, and read mask.
// Output semantic names are normalized to HLSL casing (e.g. SV_TARGET -> SV_Target).
static std::vector<SigParam> parseSignature(const uint8_t *data, uint32_t sz, const char *fourcc) {
	std::vector<SigParam> params;
	if (sz < 8) return params;

	// Determine record layout from signature chunk type
	uint32_t stride = 24;
	bool hasStream = false;
	if (std::memcmp(fourcc, "OSG5", 4) == 0) { stride = 28; hasStream = true; }
	if (std::memcmp(fourcc, "ISG1", 4) == 0 || std::memcmp(fourcc, "OSG1", 4) == 0 ||
		std::memcmp(fourcc, "PSG1", 4) == 0) { stride = 32; hasStream = true; }

	uint32_t count = readU32(data, sz, 0);
	if (count > 64) return params;

	for (uint32_t i = 0; i < count; i++) {
		uint32_t base = 8 + i * stride;
		if (!checkBounds(data, sz, base, stride)) break;

		uint32_t nameOff, semIdx, compType, reg;

		if (hasStream) {
			nameOff = readU32(data, sz, base + 4);
			semIdx = readU32(data, sz, base + 8);
			compType = readU32(data, sz, base + 16);
			reg = readU32(data, sz, base + 20);
		} else {
			nameOff = readU32(data, sz, base + 0);
			semIdx = readU32(data, sz, base + 4);
			compType = readU32(data, sz, base + 12);
			reg = readU32(data, sz, base + 16);
		}

		uint8_t mask = 0;
		uint32_t maskOff = hasStream ? (base + 24) : (base + 20);
		if (checkBounds(data, sz, maskOff, 1)) mask = data[maskOff];

		SigParam p;
		p.semanticName = readString(data, sz, nameOff);

		if (p.semanticName == "SV_POSITION") p.semanticName = "SV_Position";
		else if (p.semanticName == "SV_CLIPDISTANCE") p.semanticName = "SV_ClipDistance";
		else if (p.semanticName == "SV_CULLDISTANCE") p.semanticName = "SV_CullDistance";
		else if (p.semanticName == "SV_RENDERARRAYINDEX") p.semanticName = "SV_RenderArrayIndex";
		else if (p.semanticName == "SV_COVERAGE") p.semanticName = "SV_Coverage";
		else if (p.semanticName == "SV_TARGET") p.semanticName = "SV_Target";

		p.semanticIndex = semIdx;
		p.registerIdx = reg;

		std::string baseType = compTypeToHLSL((CompType)compType);

		uint32_t components = 0;
		if (mask & 0x01) components++;
		if (mask & 0x02) components++;
		if (mask & 0x04) components++;
		if (mask & 0x08) components++;

		if (components == 0) components = 1;

		p.typeName = baseType + (components > 1 ? std::to_string(components) : "");

		params.push_back(p);
	}

	return params;
}

// --- HLSL Generation ---

// Serialize ShaderInfo into a textual HLSL declaration listing. Output order:
// (1) resource declarations: samplers, UAVs, structured buffers, textures
// (2) struct definitions (from cbuffer field type descriptors)
// (3) cbuffer declarations with fields sorted by byte offset, packoffset annotations
// (4) InputStruct from ISGN/ISG1 signature
// (5) OutputStruct from OSGN/OSG5/OSG1 signature
static std::string generateHLSL(const ShaderInfo &info) {
	std::ostringstream o;

	// Resource declarations

	for (size_t i = 0; i < info.resources.size(); i++) {
		auto &r = info.resources[i];
		if (r.inputType == SIT_SAMPLER) {
			o << "SamplerState " << r.name << " : register(s" << r.bindPoint << ");\n";
		} else if (r.inputType == SIT_UAV_RWTYPED) {
			std::string texName = r.name.empty() ? "uav" : r.name;
			o << dimToRWTextureType(r.dimension) << " " << texName
				<< " : register(u" << r.bindPoint << ");\n";
		} else if (r.inputType == SIT_STRUCTURED) {
			std::string bufName = r.name.empty() ? "buf" : r.name;
			// Check if there's a matching struct from patch constants
			std::string structType = "";
			for (size_t si = 0; si < info.structs.size(); si++) {
				if (!info.structs[si].name.empty()) {
					// Find struct referenced by cbuffer with same name
					for (size_t ci = 0; ci < info.cbufs.size(); ci++) {
						if (info.cbufs[ci].name == r.name ||
							info.cbufs[ci].name == bufName) {
							// Check if the cbuffer has a struct-typed field
							for (size_t fi = 0; fi < info.cbufs[ci].fields.size(); fi++) {
								if (info.cbufs[ci].fields[fi].cls == SVC_STRUCT) {
									structType = info.structs[si].name;
									break;
								}
							}
							if (!structType.empty()) break;
						}
					}
					if (!structType.empty()) break;
				}
			}
			if (structType.empty()) structType = "uint"; // fallback
			o << dimToStructuredType(r.dimension) << "<" << structType << "> "
				<< bufName << " : register(t" << r.bindPoint << ");\n";
		} else if (r.inputType == SIT_BYTE_ADDRESS) {
			std::string bufName = r.name.empty() ? "byteBuf" : r.name;
			o << "ByteAddressBuffer " << bufName
				<< " : register(t" << r.bindPoint << ");\n";
		} else if (r.inputType == SIT_TEXTURE) {
			if (r.bindCount > 1) {
				for (uint32_t b = 0; b < r.bindCount; b++) {
					std::string arrName = r.name;
					if (r.name.empty()) arrName = "texture";
					o << dimToTextureType(r.dimension) << " " << arrName << "[" << b
						<< "] : register(t" << (r.bindPoint + b) << ");\n";
				}
			} else {
				std::string texName = r.name;
				if (texName.empty()) texName = "texture";
				o << dimToTextureType(r.dimension) << " " << texName
					<< " : register(t" << r.bindPoint << ");\n";
			}
		}
	}

	if (!info.resources.empty() && !info.cbufs.empty()) o << "\n";

	// Struct definitions
	std::vector<std::string> structNames;
	structNames.reserve(info.structs.size());
	for (size_t si = 0; si < info.structs.size(); si++) {
		const auto &sdef = info.structs[si];
		if (!sdef.name.empty()) {
			structNames.push_back(sdef.name);
		} else {
			structNames.push_back("UnnamedStruct");
		}
	}

	if (!info.structs.empty()) {
		for (size_t si = 0; si < info.structs.size(); si++) {
			auto &sdef = info.structs[si];
			std::string sname = structNames[si];

			o << "struct " << sname << " {\n";
			for (size_t mi = 0; mi < sdef.members.size(); mi++) {
				auto &m = sdef.members[mi];
				std::string mtype;
				if (m.cls == SVC_STRUCT) {
					// Nested structs are collected before their parent, so the
					// referenced struct is always the one immediately preceding
					// the current struct in the list.
					mtype = (si > 0) ? structNames[si - 1] : "UnnamedStruct";
				} else {
					mtype = typeToHLSL(m.cls, m.type, m.rows, m.cols);
				}
				o << "\t" << mtype << " " << m.name;
				if (m.elements > 1) o << "[" << m.elements << "]";
				o << ";\n";
			}
			o << "};\n\n";
		}
	}

	// Cbuffer declarations
	for (size_t ci = 0; ci < info.cbufs.size(); ci++) {
		auto &cb = info.cbufs[ci];
		o << "cbuffer " << cb.name << " : register(b" << cb.bindPoint << ") {\n";

		std::vector<size_t> order(cb.fields.size());
		for (size_t i = 0; i < cb.fields.size(); i++) order[i] = i;
		std::sort(order.begin(), order.end(), [&cb](size_t a, size_t b) {
			return cb.fields[a].byteOffset < cb.fields[b].byteOffset;
		});

		uint32_t cumulativeOff = 0;
		for (size_t oi = 0; oi < order.size(); oi++) {
			auto &f = cb.fields[order[oi]];
			std::string t;
			if (f.cls == SVC_STRUCT) {
				if (!info.structs.empty()) {
					auto &lastStruct = info.structs.back();
					t = lastStruct.name.empty() ? "UnnamedStruct" : lastStruct.name;
				} else {
					t = "UnnamedStruct";
				}
			} else {
				t = typeToHLSL(f.cls, f.type, f.rows, f.cols);
			}
			uint32_t fsize = getFieldByteSize(f.cls, f.type, f.rows, f.cols);
			uint32_t boff = f.byteOffset ? f.byteOffset : cumulativeOff;
			uint32_t regOff = boff / 16;
			uint32_t compOff = (boff % 16) / 4;
			o << "\t" << t << " " << f.name;
			if (f.elements > 1) o << "[" << f.elements << "]";
			o << " : packoffset(c" << regOff << ".";
			switch (compOff % 4) {
				case 0: o << "x"; break;
				case 1: o << "y"; break;
				case 2: o << "z"; break;
				default: o << "w"; break;
			}
			o << ");\n";
			cumulativeOff = boff + fsize;
		}
		o << "};\n";
		if (ci + 1 < info.cbufs.size()) o << "\n";
	}

	if (!info.cbufs.empty()) o << "\n";

	// Input signature as struct
	if (!info.inputParams.empty()) {
		o << "struct InputStruct {\n";
		uint32_t pidx = 1;
		for (size_t i = 0; i < info.inputParams.size(); i++) {
			auto &p = info.inputParams[i];
			std::string pname;
			if (p.semanticName == "SV_Position") pname = "Position";
			else pname = "param" + std::to_string(pidx++);
			o << "\t" << p.typeName << " " << pname << " : " << p.semanticName;
			if (p.semanticName == "TEXCOORD" || p.semanticIndex > 0)
				o << p.semanticIndex;
			o << ";\n";
		}
		o << "};\n\n";
	}

	// Output signature as struct
	if (!info.outputParams.empty()) {
		o << "struct OutputStruct {\n";
		for (size_t i = 0; i < info.outputParams.size(); i++) {
			auto &p = info.outputParams[i];
			o << "\t" << p.typeName << " Target" << i << " : " << p.semanticName;
			o << p.semanticIndex;
			o << ";\n";
		}
		o << "};\n";
	}

	return o.str();
}

// --- Public API ---

// Top-level entry point. Accepts a raw DXBC binary blob and returns a string of
// HLSL declarations describing the shader's interface: resources, structs, cbufs,
// and input/output signatures. Logs diagnostic messages to stderr via DriverLog.
// Returns empty string on any error (null input, bad magic, no parsable chunks).
std::string DecodeShaderVariables(const uint8_t *data, uint32_t size) {
	if (!data || size == 0) {
		DriverLog("shader-decode: null/empty input");
		return "";
	}
	if (std::memcmp(data, "DXBC", 4) != 0) {
		DriverLog("shader-decode: not a DXBC file");
		return "";
	}

	auto parts = parseContainer(data, size);
	if (parts.empty()) {
		DriverLog("shader-decode: no parts found in container");
		return "";
	}

	ShaderInfo info;

	DXBCPart rdef;
	if (findPart(parts, "RDEF", rdef))
		parseRDEF(rdef.data, rdef.size, info);

	DXBCPart sig;
	for (const char *tag : {"ISGN", "ISG1"}) {
		if (findPart(parts, tag, sig)) {
			info.inputParams = parseSignature(sig.data, sig.size, tag);
			break;
		}
	}

	for (const char *tag : {"OSG5", "OSGN", "OSG1"}) {
		if (findPart(parts, tag, sig)) {
			info.outputParams = parseSignature(sig.data, sig.size, tag);
			break;
		}
	}

	if (info.resources.empty() && info.cbufs.empty() &&
		info.inputParams.empty() && info.outputParams.empty()) {
		DriverLog("shader-decode: no shader data found");
		return "";
	}

	return generateHLSL(info);
}
