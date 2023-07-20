#pragma once

#include "Core/Core.h"

namespace vast::gfx
{
	
	class Camera
	{
	public:
		static float4x4 ComputeViewMatrix(const float3& position, const float3& target, const float3& up);
		static float4x4 ComputeProjectionMatrix(float fovY, float aspectRatio, float zNear, float zFar, bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);
		static float4x4 ComputeViewProjectionMatrix(const float4x4& view, const float4x4& proj);
	};
}