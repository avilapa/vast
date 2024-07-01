#ifndef __PER_FRAME_SHARED_H__
#define __PER_FRAME_SHARED_H__

#include "shaders_shared.h"
#include "ShaderDebug_shared.h"

struct SimpleRenderer_PerFrame
{
	float4x4 viewProjMatrix;

	s_float3 cameraPos;
	uint32 skyboxTexIdx;

	IBL_PerFrame ibl;
	ShaderDebug_PerFrame shaderDebug;
};

#if !defined(__cplusplus)
ConstantBuffer<SimpleRenderer_PerFrame> FrameConstantBuffer : register(b0, PerObjectSpace);
#endif
#endif // __PER_FRAME_SHARED_H__
