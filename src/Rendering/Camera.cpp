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

	//

	Camera::Camera(const float4x4& transform, float zNear /* = 0.001f */, float zFar /* = 1000.0f */, bool bReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
		: m_ViewMatrix()
		, m_ProjMatrix()
		, m_ViewProjMatrix()
		, m_ModelMatrix(transform)
		, m_ZNear(zNear)
		, m_ZFar(zFar)
		, m_bReverseZ(bReverseZ)
	{
		ModelChanged();
	}
	
	Camera::Camera(const float3& eye, const float3& lookAt, const float3& up, float zNear /* = 0.001f */, float zFar /* = 1000.0f */, bool bReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
		: m_ViewMatrix()
		, m_ProjMatrix()
		, m_ViewProjMatrix()
		, m_ModelMatrix()
		, m_ZNear(zNear)
		, m_ZFar(zFar)
		, m_bReverseZ(bReverseZ)
	{
		SetLookAt(eye, lookAt, up);
	}

	float4x4 Camera::GetViewMatrix() const
	{
		return m_ViewMatrix;
	}

	float4x4 Camera::GetProjectionMatrix() const
	{
		return m_ProjMatrix;
	}
	
	float4x4 Camera::GetViewProjectionMatrix() const
	{
		return m_ViewProjMatrix;
	}

	void Camera::SetLookAt(const float3& eye, const float3& lookAt, const float3& up)
	{
		m_ViewMatrix = Camera::ComputeViewMatrix(eye, lookAt, up);
		ViewChanged();
	}

	void Camera::SetTransform(const float4x4& model)
	{
		m_ModelMatrix = model;
		ModelChanged();
	}

	float4x4 Camera::GetTransform() const
	{
		return m_ModelMatrix;
	}

	void Camera::SetNearClip(float zNear)
	{
		m_ZNear = zNear;
		RegenerateProjection();
		ProjectionChanged();
	}

	void Camera::SetFarClip(float zFar)
	{
		m_ZFar = zFar;
		RegenerateProjection();
		ProjectionChanged();
	}

	void Camera::SetIsReverseZ(bool bReverseZ)
	{
		m_bReverseZ = bReverseZ;
		RegenerateProjection();
		ProjectionChanged();
	}

	float Camera::GetNearClip() const
	{
		return m_ZNear;
	}

	float Camera::GetFarClip() const
	{
		return m_ZFar;
	}

	bool Camera::GetIsReverseZ() const
	{
		return m_bReverseZ;
	}
	
	bool Camera::GetIsOrthographic() const
	{
		return false;
	}

	void Camera::ModelChanged()
	{
		m_ViewMatrix = hlslpp::inverse(m_ModelMatrix);
		m_ViewProjMatrix = Camera::ComputeViewProjectionMatrix(m_ViewMatrix, m_ProjMatrix);
	}

	void Camera::ViewChanged()
	{
		m_ModelMatrix = hlslpp::inverse(m_ViewMatrix);
		m_ViewProjMatrix = Camera::ComputeViewProjectionMatrix(m_ViewMatrix, m_ProjMatrix);
	}

	void Camera::ProjectionChanged()
	{
		m_ViewProjMatrix = Camera::ComputeViewProjectionMatrix(m_ViewMatrix, m_ProjMatrix);
	}

	//

	PerspectiveCamera::PerspectiveCamera(const float4x4& transform, float aspectRatio /* = (16.0f / 9.0f) */, float fovY /* = DEG_TO_RAD(45) */, float zNear /* = 0.001f */, float zFar /* = 1000.0f */, bool bReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
		: Camera(transform, zNear, zFar, bReverseZ)
		, m_AspectRatio(aspectRatio)
		, m_FovY(fovY)
	{
		RegenerateProjection();
		ProjectionChanged();
	}

	PerspectiveCamera::PerspectiveCamera(const float3& eye, const float3& lookAt, const float3& up, float aspectRatio /* = (16.0f / 9.0f) */, float fovY /* = DEG_TO_RAD(45) */, float zNear /* = 0.001f */, float zFar /* = 1000.0f */, bool bReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
		: Camera(eye, lookAt, up, zNear, zFar, bReverseZ)
		, m_AspectRatio(aspectRatio)
		, m_FovY(fovY)
	{
		RegenerateProjection();
		ProjectionChanged();
	}

	void PerspectiveCamera::SetAspectRatio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;
		RegenerateProjection();
		ProjectionChanged();
	}

	void PerspectiveCamera::SetFieldOfView(float fovY)
	{
		m_FovY = fovY;
		RegenerateProjection();
		ProjectionChanged();
	}

	float PerspectiveCamera::GetAspectRatio() const
	{
		return m_AspectRatio;
	}
	
	float PerspectiveCamera::GetFieldOfView() const
	{
		return m_FovY;
	}

	void PerspectiveCamera::RegenerateProjection()
	{
		m_ProjMatrix = Camera::ComputeProjectionMatrix(m_FovY, m_AspectRatio, m_ZNear, m_ZFar, m_bReverseZ);
	}

}
