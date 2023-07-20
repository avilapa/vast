#include "vastpch.h"
#include "Rendering/Camera.h"

namespace vast::gfx
{

	float4x4 Camera::ComputeViewMatrix(const float3& position, const float3& target, const float3& up)
	{
		return float4x4::look_at(position, target, up);
	}

	float4x4 Camera::ComputeProjectionMatrix(float fovY, float aspectRatio, float zNear, float zFar, bool bReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
	{
		if (bReverseZ)
		{
			// To use reverse-z we need to swap the near and far planes of the camera
			float temp = zNear;
			zNear = zFar;
			zFar = temp;
		}

		hlslpp::frustum frustum = hlslpp::frustum::field_of_view_y(fovY, aspectRatio, zNear, zFar);
		return float4x4::perspective(hlslpp::projection(frustum, hlslpp::zclip::t::zero));
	}

	float4x4 Camera::ComputeViewProjectionMatrix(const float4x4& view, const float4x4& proj)
	{
		return hlslpp::mul(view, proj);
	}
}
