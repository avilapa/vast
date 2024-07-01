#ifndef __SHADER_DEBUG_SHARED_H__
#define __SHADER_DEBUG_SHARED_H__

#include "shaders_shared.h"

struct ShaderDebug_PerFrame
{
	float4 var0;
	float4 var1;

	uint32 flags;
	uint32 _pad0, _pad1, _pad2;
};

#if !defined(__cplusplus)
#define GetDebugVar0()		FrameConstantBuffer.shaderDebug.var0
#define GetDebugVar1()		FrameConstantBuffer.shaderDebug.var1

#define GetDebugToggle(n)	((FrameConstantBuffer.shaderDebug.flags & (1 << n)) != 0)
#endif
#endif // __SHADER_DEBUG_SHARED_H__
