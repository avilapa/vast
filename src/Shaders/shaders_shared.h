#ifndef __SHARED_HLSL__
#define __SHARED_HLSL__

#if defined(__cplusplus)
using namespace vast;
#else
// Note: Shader shared types (s_) are needed when using hlslpp types due to these being aligned to
// 16 bytes for SIMD reasons. Read more: https://github.com/redorav/hlslpp/issues/53
#define s_float2	float2
#define s_float3	float3

#define uint32		uint
#define int32		int
#endif

#define PerObjectSpace space0 // TODO: Implement spaces!
#define PerFrameSpace space1 // TODO: Implement spaces!

struct UnlitCB
{
	float4x4 modelMatrix;
	uint32 vtxBufIdx;
	uint32 color;
};

struct MeshCB // TODO: Default values
{
	float4x4 modelMatrix;

	uint32 vtxBufIdx;
	uint32 colorTexIdx;
	uint32 colorSamplerIdx;
	uint32 bUseColorTex;

	s_float3 baseColor;
	float metallic;

	float roughness;
	uint32 noiseTexIdx;
};

struct IBL_PerFrame
{
	uint32 envBrdfLutIdx;
	uint32 irradianceTexIdx;
	uint32 prefilterTexIdx;
	uint32 prefilterNumMips;
};

#endif