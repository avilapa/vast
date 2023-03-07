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

#define PushConstant b999
#define PerObjectSpace space0

struct TriangleVtx
{
	s_float2 pos;
	s_float3 col;
};

struct TriangleCBV
{
	uint32 vtxBufIdx;
};

#endif