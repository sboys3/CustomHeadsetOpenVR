// HLSL function stub generated

struct UnnamedInner {
	float4x4 matProj;
	float4x4 matInvProj;
	float4x4 matRendered;
	float4x4 matInvRendered;
	float4x4 matTexDetails;
};

struct Unnamed {
	uint nFrameId;
	uint nVsyncId;
	uint nPredictedVsyncId;
	uint a0;
	uint b0;
	uint b1;
	uint b2;
	uint b3;
	UnnamedInner eye[2];
};


cbuffer DistortConstantBuffer_t : register(b0) {
	float4x4 g_matInvRendered : packoffset(c0.x);
	float4x4 g_matInvRenderedProj : packoffset(c4.x);
	float4 g_vForegroundColor : packoffset(c8.x);
	float4 g_vEdgeColor : packoffset(c9.x);
	float4 g_vColorPrescaleLinear : packoffset(c10.x);
	float3 g_vColorPrescale : packoffset(c11.x);
	float g_flTime : packoffset(c11.w);
	float g_flBlackLevel : packoffset(c12.x);
	float g_bSceneTextureRequiresGammaToLinearConversion : packoffset(c12.y);
	float2 g_vMCInvSize : packoffset(c12.z);
	float g_flMCScale : packoffset(c13.x);
	float g_flMCOffset : packoffset(c13.y);
	float2 g_vLastFrameUncorrectedInvSize : packoffset(c13.z);
	float2 g_vColorWarpUvScale : packoffset(c14.x);
	float2 g_vColorWarpUvBias : packoffset(c14.z);
	float g_flGCScale : packoffset(c15.x);
	float g_flGCOffset : packoffset(c15.y);
	float g_bMCEnabled : packoffset(c15.z);
	float g_bGhostCorrectionEnabled : packoffset(c15.w);
	uint g_nEye : packoffset(c16.x);
};

#ifndef NO_DISTORTION 
cbuffer SceneTextureIndexConstantBuffer_t : register(b2) {
	uint g_nSceneTextureIndex : packoffset(c0.x);
	Unnamed g_SceneTextureData[3] : packoffset(c1.x);
};
#endif


SamplerState g_sScene : register(s0); // can't disambiguate
//SamplerComparisonState g_sScene : register(s0); // can't disambiguate

Texture2D<float4> g_tScene0 : register(t0);
Texture2D<float4> g_tScene1 : register(t1);
Texture2D<float4> g_tScene2 : register(t2);
#ifndef NO_LAYER
Texture2D<float4> g_tLayer : register(t4);
#endif

#ifdef MURA_CORRECTION
Texture2D<float4> g_tMC : register(t5);
Texture2D<float4> g_tLastFrameUncorrected : register(t6);
Texture2D<float4> g_tGhostCorrectionTable : register(t7);
#endif





#define UV_TYPE float4
#ifdef NO_LAYER
#define UV_TYPE float2
#endif

struct InputStruct {
	float4 Position : SV_Position;
	UV_TYPE uv1 : TEXCOORD0;
	#ifndef NO_DISTORTION 
	UV_TYPE uv2 : TEXCOORD1;
	UV_TYPE uv3 : TEXCOORD2;
	#endif
	uint param4 : BLENDINDICES;
};

struct OutputStruct {
	float4 Target0 : SV_Target0;
};

// effects that involve multiple channels must be done on the input image, not on the output image.
// this is because each channel in the output gos to different locations on the display.
// instead the effects must be processed on each input image sample.
float4 inputColorProcessor(float4 color){
	#ifdef CHROMA
	float grayScale = dot(color.rgb, float3(0.299, 0.587, 0.114));
	color.rgb = lerp(grayScale, color.rgb, CHROMA);
	#endif
	#ifdef COLOR_CORRECTION_MATRIX
	// this matrix converts from linear srgb to linear display p3 on the MeganeX
	// it can also be used for any additional color corrections
	color.rgb = mul(float3x3(COLOR_CORRECTION_MATRIX), color.rgb);
	// the display p3 curve is the same as srgb curve so the gamma correction is also correct
	#endif
	return color;
}

// replacing the sampling here with something that does not have as many artifacts as bilinear may help
// #define FxaaTexLod0(t, p) tex.SampleLevel(g_sScene, p, 0.0) 
// #define FxaaTexOff(t, p, o, r) tex.SampleLevel(g_sScene, p, 0.0, o)
#define FxaaTexLod0(t, p, s) tex.Sample(g_sScene, p) 
#define FxaaTexOff(t, p, o, r, s) tex.Sample(g_sScene, p + (o * r)) 
// #define FxaaTexLod0(t, p, s) getTexel(tex, p, s) 
// #define FxaaTexOff(t, p, o, r, s) getTexel(tex, p + (o * r), s) 

// FXAA version 2
float4 FxaaPixelShader(
in float4 posPos,       // Output of FxaaVertexShader interpolated across screen.
in Texture2D<float4> tex,         // Input texture.
in float2 rcpFrame, // Constant {1.0/frameWidth, 1.0/frameHeight}.
in float2 dx, // uv derivative in the x direction
in float2 dy // uv derivative in the y direction
) {
	#define FXAA_REDUCE_MIN   (1.0/128.0)
	#define FXAA_REDUCE_MUL   (1.0/8.0)
	#define FXAA_SPAN_MAX     8.0
	// dx = normalize(dx);
	// dy = normalize(dy);
	float2 textureRez = 1.0/rcpFrame;
/*--------------------------------------------------------------------------*/
	float3 rgbNW = FxaaTexLod0(tex, posPos.zw, textureRez).xyz;
	float3 rgbNE = FxaaTexOff(tex, posPos.zw, int2(1,0), rcpFrame.xy, textureRez).xyz;
	float3 rgbSW = FxaaTexOff(tex, posPos.zw, int2(0,1), rcpFrame.xy, textureRez).xyz;
	float3 rgbSE = FxaaTexOff(tex, posPos.zw, int2(1,1), rcpFrame.xy, textureRez).xyz;
	float4 rgbM4  = FxaaTexLod0(tex, posPos.xy, textureRez).xyzw;
	float3 rgbM  = rgbM4.xyz;
	float rgbMA  = rgbM4.a;
	// return rgbM;
	// return float3(posPos.xy, 0);
/*--------------------------------------------------------------------------*/
	float3 luma = float3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);
/*--------------------------------------------------------------------------*/
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
/*--------------------------------------------------------------------------*/
	float2 dir; 
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
/*--------------------------------------------------------------------------*/
	float dirReduce = max(
		(lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
		FXAA_REDUCE_MIN);
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(float2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), 
		max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), 
		dir * rcpDirMin)) * rcpFrame.xy;
/*--------------------------------------------------------------------------*/
	float3 rgbA = (1.0/2.0) * (
		FxaaTexLod0(tex, posPos.xy + dir * (1.0/3.0 - 0.5), textureRez).xyz +
		FxaaTexLod0(tex, posPos.xy + dir * (2.0/3.0 - 0.5), textureRez).xyz);
	float3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
		FxaaTexLod0(tex, posPos.xy + dir * (0.0/3.0 - 0.5), textureRez).xyz +
		FxaaTexLod0(tex, posPos.xy + dir * (3.0/3.0 - 0.5), textureRez).xyz);
	float lumaB = dot(rgbB, luma);
	if((lumaB < lumaMin) || (lumaB > lumaMax)) return float4(rgbA, rgbMA);
	return float4(rgbB, rgbMA);
}

float4 sampleSceneTexture(in Texture2D<float4> tex, in float2 uv, in float2 dx, in float2 dy){
	#ifdef FILTER_FXAA2
	// FXAA 2
	// This uses FXAA as an upsampling method. It has some artifacts, but generally works OK.
	int2 textureSize = int2(1024,1024);
	tex.GetDimensions(textureSize.x, textureSize.y);
	float2 inverseTextureSize = float2(1.0/textureSize.x, 1.0/textureSize.y);
	#define FXAA_SUBPIX_SHIFT (1.0/4.0)
	float4 posPos = float4(uv, uv - (inverseTextureSize * (0.5 + FXAA_SUBPIX_SHIFT)));
	float4 color = FxaaPixelShader(posPos, tex, inverseTextureSize, dx, dy);
	#else
	// standard bilinear
	float4 color = tex.Sample(g_sScene, uv);
	#endif
	return inputColorProcessor(color);
}

OutputStruct main(in InputStruct IN)
{
	OutputStruct OUT = (OutputStruct)0;
	
	
	
	#ifndef NO_DISTORTION
	
	float2 uvDx = ddx(IN.uv2.xy);
	float2 uvDy = ddy(IN.uv2.xy);
	float2 uvDxOverlay = ddx(IN.uv2.zw);
	float2 uvDyOverlay = ddy(IN.uv2.zw);
	uint2 outputPixel = uint2(IN.Position.xy);
	int2 outputPixelOdd2D = uint2(frac(outputPixel/2.0f)*2.0f) % 2;
	
	#ifdef SUBPIXEL_SHIFT_MEGANEX8K
	// do subpixel offsets
	// https://www.shadertoy.com/view/Wcd3D7
	// only the y direction is done because the x direction is already done by global offsets in the UVs
	bool outputPixelOdd = outputPixelOdd2D.x == 0;
	float2 offsetAmountY = uvDy * 0.25f;
	float2 offsetAmountYOverlay = uvDyOverlay * 0.25f;
	// if(frac(g_flTime)>0.5){
	// 	offsetAmountY = 100;
	// }
	// I am not sure why the red subpixel is flipped vs the shadertoy, but it seems correct in testing
	if(outputPixelOdd){
		IN.uv1.xy += -offsetAmountY;
		IN.uv2.xy +=  offsetAmountY;
		IN.uv3.xy += -offsetAmountY;
		
		IN.uv1.zw += -offsetAmountYOverlay;
		IN.uv2.zw +=  offsetAmountYOverlay;
		IN.uv3.zw += -offsetAmountYOverlay;
	}else{
		IN.uv1.xy +=  offsetAmountY;
		IN.uv2.xy += -offsetAmountY;
		IN.uv3.xy +=  offsetAmountY;
		
		IN.uv1.zw +=  offsetAmountYOverlay;
		IN.uv2.zw += -offsetAmountYOverlay;
		IN.uv3.zw +=  offsetAmountYOverlay;
	}
	#endif
	
	#ifdef SUBPIXEL_SHIFT_VIVE
	// do subpixel offsets
	// https://www.shadertoy.com/view/Wcd3D7
	// only the x direction is done because the y direction is already done by global offsets in the UVs
	bool outputPixelOdd = outputPixelOdd2D.x != outputPixelOdd2D.y;
	float2 offsetAmountX = uvDx * 0.5f;
	float2 offsetAmountXOverlay = uvDxOverlay * 0.5f;
	// if(frac(g_flTime)>0.5){
	// 	offsetAmountY = 100;
	// }
	if(outputPixelOdd){
		IN.uv1.xy +=  offsetAmountX;
		IN.uv3.xy += -offsetAmountX;
		
		IN.uv1.zw +=  offsetAmountXOverlay;
		IN.uv3.zw += -offsetAmountXOverlay;
	}else{
		IN.uv1.xy += -offsetAmountX;
		IN.uv3.xy +=  offsetAmountX;
		
		IN.uv1.zw += -offsetAmountXOverlay;
		IN.uv3.zw +=  offsetAmountXOverlay;
	}
	#endif
	
	#endif // NO_DISTORTION
	
	
	// if(IN.uv1.x < 0 || IN.uv1.x > 1 || IN.uv1.y < 0 || IN.uv1.y > 1 || IN.uv2.x < 0 || IN.uv2.x > 1 || IN.uv2.y < 0 || IN.uv2.y > 1 || IN.uv3.x < 0 || IN.uv3.x > 1 || IN.uv3.y < 0 || IN.uv3.y > 1){
	// 	OUT.Target0 = float4(0,0,0,1);
	// 	return OUT;
	// }
	
	
	float4 col = 1;
	#ifndef NO_DISTORTION
	[forcecase] switch (IN.param4){
		case 0:{
			col.x = sampleSceneTexture(g_tScene0, IN.uv1.xy, uvDx, uvDy).x;
			col.y = sampleSceneTexture(g_tScene0, IN.uv2.xy, uvDx, uvDy).y;
			col.z = sampleSceneTexture(g_tScene0, IN.uv3.xy, uvDx, uvDy).z;
			break;
		}
		case 1:{
			col.x = sampleSceneTexture(g_tScene1, IN.uv1.xy, uvDx, uvDy).x;
			col.y = sampleSceneTexture(g_tScene1, IN.uv2.xy, uvDx, uvDy).y;
			col.z = sampleSceneTexture(g_tScene1, IN.uv3.xy, uvDx, uvDy).z;
			break;
		}
		default:{
			col.x = sampleSceneTexture(g_tScene2, IN.uv1.xy, uvDx, uvDy).x;
			col.y = sampleSceneTexture(g_tScene2, IN.uv2.xy, uvDx, uvDy).y;
			col.z = sampleSceneTexture(g_tScene2, IN.uv3.xy, uvDx, uvDy).z;
		break;
	}}
	#else
	[forcecase] switch (IN.param4){
		case 0:{
			col = sampleSceneTexture(g_tScene0, IN.uv1.xy, uvDx, uvDy);
			break;
		}
		case 1:{
			col = sampleSceneTexture(g_tScene1, IN.uv1.xy, uvDx, uvDy);
			break;
		}
		default:{
			col = sampleSceneTexture(g_tScene2, IN.uv1.xy, uvDx, uvDy);
		break;
	}}
	#endif
	
	if(g_bSceneTextureRequiresGammaToLinearConversion != 0){
		// gamma to linear conversion
		col.rgb = pow(col.rgb, 2.2); 
	}
	
	#ifndef NO_DISTORTION 
	// clip the edges of the texture as sometimes the UVs only cover a section of a larger texture
	float4 textureRange = g_SceneTextureData[IN.param4].eye[g_nEye].matTexDetails._m20_m21_m22_m23;
	float2 redInRange = abs(IN.uv1.xy - textureRange.xy) <= textureRange.zw ? 1 : 0;
	float2 greenInRange = abs(IN.uv2.xy - textureRange.xy) <= textureRange.zw ? 1 : 0;
	float2 blueInRange = abs(IN.uv3.xy - textureRange.xy) <= textureRange.zw ? 1 : 0;
	float3 inRangeMultiplier = float3(redInRange.x * redInRange.y, greenInRange.x * greenInRange.y, blueInRange.x * blueInRange.y);
	col.rgb = col.rgb * inRangeMultiplier;
	inRangeMultiplier = 1 - inRangeMultiplier;
	#endif
	
	
	#ifndef NO_LAYER
 	// sample and combine existing overlay
 	float2 layerRA = sampleSceneTexture(g_tLayer, IN.uv1.zw, uvDxOverlay, uvDyOverlay).ra;
 	float2 layerGA = sampleSceneTexture(g_tLayer, IN.uv2.zw, uvDxOverlay, uvDyOverlay).ga;
 	float2 layerBA = sampleSceneTexture(g_tLayer, IN.uv3.zw, uvDxOverlay, uvDyOverlay).ba;
	float3 layerColors = float3(layerRA.x, layerGA.x, layerBA.x);
	float3 layerAlphas = float3(layerRA.y, layerGA.y, layerBA.y);
	
	// col.r = lerp(col.r, layerRA.x, layerRA.y);
	// col.g = lerp(col.g, layerGA.x, layerGA.y);
	// col.b = lerp(col.b, layerBA.x, layerBA.y);
	// alpha might be premultiplied
	// col.r = col.r * (1 - layerRA.y) + layerRA.x;
	// col.g = col.g * (1 - layerGA.y) + layerGA.x;
	// col.b = col.b * (1 - layerBA.y) + layerBA.x;
	col.rgb = col.rgb * (1 - layerAlphas) + layerColors;
	#endif
	
	#ifndef NO_DISTORTION 
	#ifndef NO_LAYER
	// make the color very black if there is no overlay and it is outside the uv range
	// this prevents effects from brightening up the outside of the texture.
	col.rgb += layerAlphas > 0 ? 0 : -1000 * inRangeMultiplier;
	#else
	col.rgb += -1000 * inRangeMultiplier
	#endif
	#endif
	
	// contrast before gamma
	#ifdef CONTRAST_LINEAR
	#ifdef CONTRAST_MULTIPLIER
	col.rgb *= CONTRAST_MULTIPLIER;
	#endif
	#ifdef CONTRAST_OFFSET
	col.rgb += CONTRAST_OFFSET;
	#endif
	#endif
	
	#ifdef CONTRAST_PER_EYE_LINEAR
	if(g_nEye == 0){
		#ifdef CONTRAST_MULTIPLIER_LEFT
		col.rgb *= CONTRAST_MULTIPLIER_LEFT;
		#endif
		#ifdef CONTRAST_OFFSET_LEFT
		col.rgb += CONTRAST_OFFSET_LEFT;
		#endif
	}else{
		#ifdef CONTRAST_MULTIPLIER_RIGHT
		col.rgb *= CONTRAST_MULTIPLIER_RIGHT;
		#endif
		#ifdef CONTRAST_OFFSET_RIGHT
		col.rgb += CONTRAST_OFFSET_RIGHT;
		#endif
	}
	#endif
	
	// color multiplier
	col *= g_vColorPrescaleLinear;
	
	// tint multiplier
	#ifdef COLOR_MULTIPLIER
		col.rgb *= float3(COLOR_MULTIPLIER);
	#endif
	
	#ifdef LENS_COLOR_CORRECTION
	#ifdef MEGANEX8K
	// correct for warmer colored center of the lens on the MeganeX
	#ifdef OUTPUT_RESOLUTION_X
	// distance from 0 to silghtly above 1 from the display not being square
	float2 scaledUV = (float2(outputPixel.x % OUTPUT_RESOLUTION_X, outputPixel.y) / float2(OUTPUT_RESOLUTION_X, OUTPUT_RESOLUTION_Y) - 0.5);
	scaledUV.y *= (float)OUTPUT_RESOLUTION_Y / (float)OUTPUT_RESOLUTION_X; // make it square
	float distanceFromCenter = length(scaledUV) * 2;
	#else
	// fallback to this if the resolution is not defined, but this scales with FOV
	float distanceFromCenter = length(IN.uv2.zw - 0.5) * 2;
	#endif
	// try 1
	// col.b *= 1 - min(pow(distanceFromCenter, 2), 0.15) * 1.5;
	// col.rg *= 0.9;
	// try 2
	// col.rg *= 0.9 + min(pow(distanceFromCenter, 2), 0.15) * 1.5;
	// try 3
	float scaledDistanceFromCenter = distanceFromCenter * 0.37;
	float sideAmount = min(scaledDistanceFromCenter * scaledDistanceFromCenter, 0.1);
	col.rg *= 0.9 + sideAmount * 0.75;
	col.b *= 1 - sideAmount * 0.5;
	// col = distanceFromCenter;
	// col = distanceFromCenter > 0.5 ? 1 : 0;
	// float fadeInPoint = 0.5;
	// float fadeOutPoint = 0.9;
	// float fadeCenterPoint = (fadeInPoint + fadeOutPoint) / 2;
	// col.rgb *= 1 - lerp(pow(smoothstep(fadeInPoint, fadeOutPoint, distanceFromCenter), 3), pow(smoothstep(fadeInPoint, fadeOutPoint, distanceFromCenter), 0.2), smoothstep((fadeInPoint - fadeCenterPoint) * 0.7 + fadeCenterPoint, (fadeOutPoint - fadeCenterPoint) * 0.7 + fadeCenterPoint, distanceFromCenter));
	#endif
	#endif
	
	// gamma is not done if this is not a layer shader
	#ifndef NO_LAYER
	// srgb has a small linear section at the beginning that decreases the resolution of black colors
	float3 linearColor = col.rgb;
	bool3 cutoff = linearColor > 0.0031308;
	float3 linearSection = linearColor * 12.92;
	float3 gammaSection = pow(linearColor, 1.0 / 2.4) * 1.055 - 0.055;
	col.rgb = lerp(linearSection, gammaSection, cutoff);
	#endif
	
	// allow GAMMA to modify the srgb curve
	#ifdef GAMMA
	col.rgb *= pow(col.rgb, 1.0 / GAMMA - 1.0 / 2.2);
	#endif
	
	
	
	
	// contrast after gamma
	#ifndef CONTRAST_LINEAR
	#ifdef CONTRAST_MULTIPLIER
	col.rgb *= CONTRAST_MULTIPLIER;
	#endif
	#ifdef CONTRAST_OFFSET
	col.rgb += CONTRAST_OFFSET;
	#endif
	#endif
	
	#ifndef CONTRAST_PER_EYE_LINEAR
	if(g_nEye == 0){
		#ifdef CONTRAST_MULTIPLIER_LEFT
		col.rgb *= CONTRAST_MULTIPLIER_LEFT;
		#endif
		#ifdef CONTRAST_OFFSET_LEFT
		col.rgb += CONTRAST_OFFSET_LEFT;
		#endif
	}else{
		#ifdef CONTRAST_MULTIPLIER_RIGHT
		col.rgb *= CONTRAST_MULTIPLIER_RIGHT;
		#endif
		#ifdef CONTRAST_OFFSET_RIGHT
		col.rgb += CONTRAST_OFFSET_RIGHT;
		#endif
	}
	#endif
	
	
	#ifdef MURA_CORRECTION
	#ifndef DISABLE_MURA_CORRECTION
	if(g_bMCEnabled != 0){
		float muraOffset = (g_tMC.SampleLevel(g_sScene, IN.Position.xy * g_vMCInvSize, float(0)).x + g_flMCOffset) * g_flMCScale;
		// this is my own tweak to make the correction better in dark scenes
		// col.rgb += muraOffset * sqrt(col.rgb);
		// I think this is what the default is
		col.g += muraOffset;
	}
	// TODO: ghost correction
	#endif
	#endif
	
	#ifndef DISABLE_BLACK_LEVELS
	// does not crunch blacks
	// col.rgb = lerp(max(0, col.rgb), 1, g_flBlackLevel);
	// this is how it is in the default shader but it crushes blacks
	col.rgb = max(col.rgb, g_flBlackLevel);
	#endif
	
	#ifndef NO_DISTORTION
	#ifdef DITHER_10BIT
	// dithers a 10 bit input to be displayed on a 8 bit panel temporally over 4 frames and spatially over 4 pixels
	// this is also known as Frame Rate Control (FRC)
	uint pixelId = outputPixelOdd2D.x + outputPixelOdd2D.y * 2;
	uint frameId = g_SceneTextureData[IN.param4].nVsyncId + 1; // +1 because 1 and 3 are the least intrusive and it may not increment without a game running
	// frameId = 1;
	pixelId = (pixelId + frameId) % 4;
	// pixelId = 0;
	col.rgb = floor(col.rgb * 255 + pixelId / 4.0) / 255;
	#endif
	#endif
	
	#ifdef AFTER_TEST
	#include "distort_ps_layered_after_test.hlsl"
	#endif
	
	// set sub-pixels outside of uvs to zero to prevent fringing on the edges
	// float2 uv = IN.uv1.zw;
	// if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
	// 	col.r = 0;
	// }
	// uv = IN.uv2.zw;
	// if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
	// 	col.g = 0;
	// }
	// uv = IN.uv3.zw;
	// if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
	// 	col.b = 0;
	// }
	
	// odd pixels
	// col.rgb = outputPixelOdd ? 0.5 : 0.2;
	// checkerboard pattern
	// col.rgb = outputPixelOdd2D.x != outputPixelOdd2D.y ? 0.9 : 0.1; 
	// blocks of 20 pixels
	// col.rg = float2(int2(IN.Position.xy) % 20) / 20.0;
	
	// eye-patch
	// if(g_nEye == 1){col = 0;}
		
	
	OUT.Target0 = col;
	
	return OUT;
}


