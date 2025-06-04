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

OutputStruct main(in InputStruct IN)
{
	OutputStruct OUT = (OutputStruct)0;
	
	
	
	#ifdef MEGANEX8K_SUBPIXEL_SHIFT
	//float2 uvDx = ddx(IN.uv2.xy);
	float2 uvDy = ddy(IN.uv2.xy);
	
	uint2 outputPixel = uint2(IN.Position.xy);
	
	// do subpixel offsets
	// https://www.shadertoy.com/view/Wcd3D7
	// only the y direction is done because the x direction is already done by global offsets in the UVs
	int2 outputPixelOdd2D = int2(frac(outputPixel/2.0f)*2.0f) % 2;
	bool outputPixelOdd = outputPixelOdd2D.x == 0;
	float offsetAmountY = 0.25f;
	// if(frac(g_flTime)>0.5){
	// 	offsetAmountY = 100;
	// }
	if(outputPixelOdd){
		IN.uv1.xy += uvDy *  offsetAmountY;
		IN.uv2.xy += uvDy *  offsetAmountY;
		IN.uv3.xy += uvDy * -offsetAmountY;
	}else{
		IN.uv1.xy += uvDy * -offsetAmountY;
		IN.uv2.xy += uvDy * -offsetAmountY;
		IN.uv3.xy += uvDy *  offsetAmountY;
	}
	#endif
	
	// if(IN.uv1.x < 0 || IN.uv1.x > 1 || IN.uv1.y < 0 || IN.uv1.y > 1 || IN.uv2.x < 0 || IN.uv2.x > 1 || IN.uv2.y < 0 || IN.uv2.y > 1 || IN.uv3.x < 0 || IN.uv3.x > 1 || IN.uv3.y < 0 || IN.uv3.y > 1){
	// 	OUT.Target0 = float4(0,0,0,1);
	// 	return OUT;
	// }
	
	float4 col = 1;
	[forcecase] switch (IN.param4){
		case 0:{
			col.x = g_tScene0.Sample(g_sScene, IN.uv1.xy).x;
			col.y = g_tScene0.Sample(g_sScene, IN.uv2.xy).y;
			col.z = g_tScene0.Sample(g_sScene, IN.uv3.xy).z;
			break;
		}
		case 1:{
			col.x = g_tScene1.Sample(g_sScene, IN.uv1.xy).x;
			col.y = g_tScene1.Sample(g_sScene, IN.uv2.xy).y;
			col.z = g_tScene1.Sample(g_sScene, IN.uv3.xy).z;
			break;
		}
		default:{
			col.x = g_tScene2.Sample(g_sScene, IN.uv1.xy).x;
			col.y = g_tScene2.Sample(g_sScene, IN.uv2.xy).y;
			col.z = g_tScene2.Sample(g_sScene, IN.uv3.xy).z;
		break;
	}}
	
	if(g_bSceneTextureRequiresGammaToLinearConversion != 0){
		// gamma to linear conversion
		col.rgb = pow(col.rgb, 2.2); 
	}
	
	
	
	// sample and combine existing overlay
	float2 layerRA = g_tLayer.Sample(g_sScene, IN.uv1.zw).ra;
	float2 layerGA = g_tLayer.Sample(g_sScene, IN.uv2.zw).ga;
	float2 layerBA = g_tLayer.Sample(g_sScene, IN.uv3.zw).ba;
	
	// col.r = lerp(col.r, layerRA.x, layerRA.y);
	// col.g = lerp(col.g, layerGA.x, layerGA.y);
	// col.b = lerp(col.b, layerBA.x, layerBA.y);
	// alpha might be premultiplied
	col.r = col.r * (1 - layerRA.y) + layerRA.x;
	col.g = col.g * (1 - layerGA.y) + layerGA.x;
	col.b = col.b * (1 - layerBA.y) + layerBA.x;
	
	
	// post processing
	col *= g_vColorPrescaleLinear;
	col.rgb = lerp(col.rgb, 1, g_flBlackLevel);
	// convert to gamma 2.2
	col.rgb = pow(col.rgb, 1.0 / 2.2);
	
	// odd pixels
	// col.rgb = outputPixelOdd ? 0.5 : 0.2;
	// checkerboard pattern
	// col.rgb = outputPixelOdd2D.x != outputPixelOdd2D.y ? 0.9 : 0.1; 
	// blocks of 20 pixels
	// col.rg = float2(int2(IN.Position.xy) % 20) / 20.0;
	
	// eye-patch
	//if(g_nEye == 1){col = 0;}
		
	
	OUT.Target0 = col;
	
	return OUT;
}


