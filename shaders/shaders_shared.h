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

#define BRDF_IRRADIANCE_TEX_RES 64

struct Vtx3fPos3fNormal2fUv
{
	s_float3 pos;
	s_float3 normal;
	s_float2 uv;
};

struct MeshCB // TODO: Default values
{
	float4x4 model;

	uint32 vtxBufIdx;
	uint32 colorTexIdx;
	uint32 colorSamplerIdx;
};

struct SimpleRenderer_PerFrame
{
	float4x4 viewProjMatrix;
	s_float3 cameraPos;
};

#endif