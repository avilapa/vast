#pragma once

#include "Core/Core.h"

namespace vast::gfx
{
	
	class Camera
	{
	public:
		Camera(const float4x4& transform, 
			float zNear = 0.001f, float zFar = 1000.0f, 
			bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);

		Camera(const float3& eye, const float3& lookAt, const float3& up,
			float zNear = 0.001f, float zFar = 1000.0f, 
			bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);
		
		float4x4 GetViewMatrix() const;
		float4x4 GetProjectionMatrix() const;
		float4x4 GetViewProjectionMatrix() const;

		float4x4 GetTransform() const;
		void SetTransform(const float4x4& model);

		void SetLookAt(const float3& eye, const float3& lookAt, const float3& up);

	protected:
		virtual void RegenerateProjection() = 0;

		void ModelChanged();
		void ViewChanged();
		void ProjectionChanged();

		float4x4 m_ViewMatrix;
		float4x4 m_ProjMatrix;
		float4x4 m_ViewProjMatrix;

		float4x4 m_ModelMatrix;

		float m_ZNear;
		float m_ZFar;
		bool m_bReverseZ;

	public:
		static float4x4 ComputeViewMatrix(const float3& position, const float3& target, const float3& up);
		static float4x4 ComputeProjectionMatrix(float fovY, float aspectRatio, float zNear, float zFar, bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);
		static float4x4 ComputeViewProjectionMatrix(const float4x4& view, const float4x4& proj);
	};

	class PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera(const float4x4& transform, 
			float aspectRatio = (16.0f / 9.0f), float fovY = DEG_TO_RAD(45), 
			float zNear = 0.001f, float zFar = 1000.0f, 
			bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);

		PerspectiveCamera(const float3& eye, const float3& lookAt, const float3& up,
			float aspectRatio = (16.0f / 9.0f), float fovY = DEG_TO_RAD(45), 
			float zNear = 0.001f, float zFar = 1000.0f, 
			bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);

	protected:
		void RegenerateProjection() override;

		float m_AspectRatio;
		float m_FovY;
	};
}