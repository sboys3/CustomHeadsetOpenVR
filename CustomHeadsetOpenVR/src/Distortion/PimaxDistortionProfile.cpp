#include "PimaxDistortionProfile.h"
#include <algorithm>

#define NOMINMAX
#include "../Helpers/PimaxCommon.h"

void PimaxDistortionProfile::Initialize() {
    pvr_getEyeRenderInfo(PimaxCommon::GetPvrSession(), pvrEye_Left, &eyeInfo[pvrEye_Left]);
    pvr_getEyeRenderInfo(PimaxCommon::GetPvrSession(), pvrEye_Right, &eyeInfo[pvrEye_Right]);
    pvr_getFovTextureSize(PimaxCommon::GetPvrSession(), pvrEye_Left, eyeInfo[pvrEye_Left].Fov, 1.f, &viewportSize);
}

void PimaxDistortionProfile::GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) {
    *pfLeft = -eyeInfo[eEye].Fov.LeftTan;
    *pfRight = eyeInfo[eEye].Fov.RightTan;
    // Top and bottom are backwards per SteamVR documentation.
    *pfTop = eyeInfo[eEye].Fov.DownTan;
    *pfBottom = -eyeInfo[eEye].Fov.UpTan;
}

Point2D PimaxDistortionProfile::ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) {
    pvrVector2f outUV[3];

    const float minResolution = (float)std::min(resolutionX, resolutionY);
    fU = (fU * minResolution / (2.0f * resolutionY)) + 0.5f;
    fV = (fV * minResolution / (2.0f * resolutionX)) + 0.5f;

    pvr_getHmdDistortedUV(PimaxCommon::GetPvrSession(), (pvrEyeType)eEye, { fU, fV }, outUV);

    float fX = (outUV[colorChannel].x - 0.5f) * 2.f;
    float fY = (outUV[colorChannel].y - 0.5f) * 2.f;

    return { fX, fY };
}

void PimaxDistortionProfile::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {
    *pnWidth = (uint32_t)viewportSize.w;
    *pnWidth = (*pnWidth + 3) / 4 * 4;
    *pnHeight = (uint32_t)viewportSize.h;
    *pnHeight = (*pnHeight + 3) / 4 * 4;
}
