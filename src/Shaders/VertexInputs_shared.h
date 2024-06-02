#ifndef __VERTEX_INPUTS_SHARED_H__
#define __VERTEX_INPUTS_SHARED_H__

#if defined(__cplusplus)
using namespace vast;
#else
#ifndef s_float2
#define s_float2	float2
#endif
#ifndef s_float3
#define s_float3	float3
#endif
#endif

struct Vtx3fPos
{
	s_float3 pos;
};

struct Vtx3fPos3fNormal
{
	s_float3 pos;
	s_float3 normal;
};

struct Vtx3fPos3fNormal2fUv
{
	s_float3 pos;
	s_float3 normal;
	s_float2 uv;
};

#endif // __VERTEX_INPUTS_SHARED_H__
