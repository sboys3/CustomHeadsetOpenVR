#ifdef PVR_EXISTS

#include "PimaxDistortionProfile.h"
#include <algorithm>

#define NOMINMAX
#include "../Helpers/PimaxCommon.h"

void PimaxDistortionProfile::GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfBottom, float* pfTop) {
    pvrEyeRenderInfo eyeInfo = {};
    pvr_getEyeRenderInfo(PimaxCommon::GetPvrSession(), eEye == vr::Eye_Left ? pvrEye_Left : pvrEye_Right, &eyeInfo);

    *pfLeft = -eyeInfo.Fov.LeftTan;
    *pfRight = eyeInfo.Fov.RightTan;
    // Top and bottom are backwards per SteamVR documentation.
    *pfTop = eyeInfo.Fov.DownTan;
    *pfBottom = -eyeInfo.Fov.UpTan;
}

Point2D PimaxDistortionProfile::ComputeDistortion(vr::EVREye eEye, ColorChannel colorChannel, float fU, float fV) {
    pvrVector2f outUV[3];
    
    Config::BaseHeadsetConfig& headsetConfig = PimaxCommon::GetHeadsetConfigDefault();
    // Counteract the display rotation and scaling applied in BaseHeadset::transformUV.
    // Inverse operations applied in reverse order of the forward transform.
    // Un-scale non-square resolution
    const float minResolution = (float)std::min(resolutionX, resolutionY);
    float displayRotation = (float)headsetConfig.displayRotation;
    fU *= minResolution / resolutionX;
    fV *= minResolution / resolutionY;
    // Counteract negation for 180 and 270 rotation
    if(displayRotation == 2 || displayRotation == 3){
        fU *= -1;
        fV *= -1;
    }
    // Counteract swap for 90 and 270 rotation
    if(displayRotation == 1 || displayRotation == 3){
        float tmp = fU;
        if(eEye == vr::Eye_Left){
            fU = fV;
            fV = -tmp;
        }else{
            fU = -fV;
            fV = tmp;
        }
    }
    // Convert from [-1,1] back to [0,1] and adjust for resolution aspect
    fU = fU / 2.0f + 0.5f;
    fV = fV / 2.0f + 0.5f;
    
    pvr_getHmdDistortedUV(PimaxCommon::GetPvrSession(), (pvrEyeType)eEye, { fU, fV }, outUV);

    float fX = (outUV[colorChannel].x - 0.5f) * 2.f;
    float fY = (outUV[colorChannel].y - 0.5f) * 2.f;

    return { fX, fY };
}

void PimaxDistortionProfile::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {
    pvrEyeRenderInfo eyeInfo = {};
    pvr_getEyeRenderInfo(PimaxCommon::GetPvrSession(), pvrEye_Left, &eyeInfo);
    pvrSizei viewportSize = {};
    pvr_getFovTextureSize(PimaxCommon::GetPvrSession(), pvrEye_Left, eyeInfo.Fov, 1.f, &viewportSize);

    *pnWidth = (uint32_t)viewportSize.w;
    *pnWidth = (*pnWidth + 3) / 4 * 4;
    *pnHeight = (uint32_t)viewportSize.h;
    *pnHeight = (*pnHeight + 3) / 4 * 4;
}

#endif // PVR_EXISTS
