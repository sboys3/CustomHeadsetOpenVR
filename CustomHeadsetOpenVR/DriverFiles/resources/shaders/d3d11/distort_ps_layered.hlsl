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

cbuffer SceneTextureIndexConstantBuffer_t : register(b2) {
	uint g_nSceneTextureIndex : packoffset(c0.x);
	Unnamed g_SceneTextureData[3] : packoffset(c1.x);
};


SamplerState g_sScene : register(s0); // can't disambiguate
//SamplerComparisonState g_sScene : register(s0); // can't disambiguate

Texture2D<float4> g_tScene0 : register(t0);
Texture2D<float4> g_tScene1 : register(t1);
Texture2D<float4> g_tScene2 : register(t2);
Texture2D<float4> g_tLayer : register(t4);

#ifdef MURA_CORRECTION
Texture2D<float4> g_tMC : register(t5);
Texture2D<float4> g_tLastFrameUncorrected : register(t6);
Texture2D<float4> g_tGhostCorrectionTable : register(t7);
#endif







struct InputStruct {
	float4 Position : SV_Position;
	float4 uv1 : TEXCOORD0;
	float4 uv2 : TEXCOORD1;
	float4 uv3 : TEXCOORD2;
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

OutputStruct main(in InputStruct IN)
{
	OutputStruct OUT = (OutputStruct)0;
	
	
	
	
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
	if(outputPixelOdd){
		IN.uv1.xy +=  offsetAmountY;
		IN.uv2.xy +=  offsetAmountY;
		IN.uv3.xy += -offsetAmountY;
		
		IN.uv1.zw +=  offsetAmountYOverlay;
		IN.uv2.zw +=  offsetAmountYOverlay;
		IN.uv3.zw += -offsetAmountYOverlay;
	}else{
		IN.uv1.xy += -offsetAmountY;
		IN.uv2.xy += -offsetAmountY;
		IN.uv3.xy +=  offsetAmountY;
		
		IN.uv1.zw += -offsetAmountYOverlay;
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
	
	// if(IN.uv1.x < 0 || IN.uv1.x > 1 || IN.uv1.y < 0 || IN.uv1.y > 1 || IN.uv2.x < 0 || IN.uv2.x > 1 || IN.uv2.y < 0 || IN.uv2.y > 1 || IN.uv3.x < 0 || IN.uv3.x > 1 || IN.uv3.y < 0 || IN.uv3.y > 1){
	// 	OUT.Target0 = float4(0,0,0,1);
	// 	return OUT;
	// }
	
	
	float4 col = 1;
	[forcecase] switch (IN.param4){
		case 0:{
			col.x = inputColorProcessor(g_tScene0.Sample(g_sScene, IN.uv1.xy)).x;
			col.y = inputColorProcessor(g_tScene0.Sample(g_sScene, IN.uv2.xy)).y;
			col.z = inputColorProcessor(g_tScene0.Sample(g_sScene, IN.uv3.xy)).z;
			break;
		}
		case 1:{
			col.x = inputColorProcessor(g_tScene1.Sample(g_sScene, IN.uv1.xy)).x;
			col.y = inputColorProcessor(g_tScene1.Sample(g_sScene, IN.uv2.xy)).y;
			col.z = inputColorProcessor(g_tScene1.Sample(g_sScene, IN.uv3.xy)).z;
			break;
		}
		default:{
			col.x = inputColorProcessor(g_tScene2.Sample(g_sScene, IN.uv1.xy)).x;
			col.y = inputColorProcessor(g_tScene2.Sample(g_sScene, IN.uv2.xy)).y;
			col.z = inputColorProcessor(g_tScene2.Sample(g_sScene, IN.uv3.xy)).z;
		break;
	}}
	
	if(g_bSceneTextureRequiresGammaToLinearConversion != 0){
		// gamma to linear conversion
		col.rgb = pow(col.rgb, 2.2); 
	}
	
	
	
	// sample and combine existing overlay
	float2 layerRA = inputColorProcessor(g_tLayer.Sample(g_sScene, IN.uv1.zw)).ra;
	float2 layerGA = inputColorProcessor(g_tLayer.Sample(g_sScene, IN.uv2.zw)).ga;
	float2 layerBA = inputColorProcessor(g_tLayer.Sample(g_sScene, IN.uv3.zw)).ba;
	
	// col.r = lerp(col.r, layerRA.x, layerRA.y);
	// col.g = lerp(col.g, layerGA.x, layerGA.y);
	// col.b = lerp(col.b, layerBA.x, layerBA.y);
	// alpha might be premultiplied
	col.r = col.r * (1 - layerRA.y) + layerRA.x;
	col.g = col.g * (1 - layerGA.y) + layerGA.x;
	col.b = col.b * (1 - layerBA.y) + layerBA.x;
	
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
	
	// post processing
	col *= g_vColorPrescaleLinear;
	
	
	// srgb has a small linear section at the beginning that decreases the resolution of black colors
	float3 linearColor = col.rgb;
	bool3 cutoff = linearColor > 0.0031308;
    float3 linearSection = linearColor * 12.92;
    float3 gammaSection = pow(linearColor, 1.0 / 2.4) * 1.055 - 0.055;
	col.rgb = lerp(linearSection, gammaSection, cutoff);
	
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

	
	// set sub-pixels outside of uvs to zero to prevent fringing on the edges
	// prefer the game uvs if they are non zero
	float2 uv = IN.uv1.zw;
	if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
		col.r = 0;
	}
	uv = IN.uv2.zw;
	if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
		col.g = 0;
	}
	uv = IN.uv3.zw;
	if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0){
		col.b = 0;
	}
	
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


