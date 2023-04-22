#pragma once

#include "SampleBase.h"
#include "shaders_shared.h"

static Array<Vtx3fPos3fNormal2fUv, 36> s_CubeVertexData =
{ {
	{{ 1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 0.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
	{{-1.0f,-1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
	{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
	{{-1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
	{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
	{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
	{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
	{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
	{{ 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 1.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
	{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }},
} };

class Hello3D final : public SampleBase3D
{
private:
	gfx::PipelineHandle m_MeshPso;
	gfx::ShaderResourceProxy m_MeshCbvBufProxy;
	gfx::BufferHandle m_MeshVtxBuf;
	gfx::BufferHandle m_MeshCbvBuf;
	gfx::TextureHandle m_MeshColorTex;
	MeshCB m_MeshCB;

public:
	Hello3D(gfx::GraphicsContext& ctx) : SampleBase3D(ctx)
	{
		auto colorTargetFormat = m_GraphicsContext.GetTextureFormat(m_ColorRT);
		auto depthTargetFormat = gfx::Format::D32_FLOAT; // m_GraphicsContext.GetTextureFormat(m_DepthRT); // TODO: Currently returns typeless

		gfx::RenderPassLayout colorDepthPass =
		{
			{ colorTargetFormat, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, gfx::ResourceState::SHADER_RESOURCE },
			{ depthTargetFormat, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, gfx::ResourceState::NONE }
		};

		m_MeshPso = m_GraphicsContext.CreatePipeline(gfx::PipelineDesc::Builder()
			.VS("mesh.hlsl", "VS_Main")
			.PS("mesh.hlsl", "PS_Main")
			.RenderPass(colorDepthPass));
		m_MeshCbvBufProxy = m_GraphicsContext.LookupShaderResource(m_MeshPso, "CB");

		auto vtxBufDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(s_CubeVertexData)).Stride(sizeof(s_CubeVertexData[0]))
			.ViewFlags(gfx::BufferViewFlags::SRV)
			.CpuAccess(gfx::BufferCpuAccess::NONE)
			.IsRawAccess(true);
		m_MeshVtxBuf = m_GraphicsContext.CreateBuffer(vtxBufDesc, &s_CubeVertexData, sizeof(s_CubeVertexData));

		m_MeshColorTex = m_GraphicsContext.CreateTexture("image.tga");

		auto windowSize = m_GraphicsContext.GetSwapChainSize();
		float fieldOfView = float(PI) / 4.0f;
		float aspectRatio = (float)windowSize.x / (float)windowSize.y;
		m_MeshCB =
		{
			float4x4(),
			float4x4::look_at({ -3.0f, 3.0f, -8.0f }, float3(0), float3(0, 1, 0)),
			float4x4::perspective(hlslpp::projection(hlslpp::frustum::field_of_view_x(fieldOfView, aspectRatio, 0.001f, 1000.0f), hlslpp::zclip::t::zero)),
			{ -3.0f, 3.0f, -8.0f },
			m_GraphicsContext.GetBindlessIndex(m_MeshVtxBuf),
			m_GraphicsContext.GetBindlessIndex(m_MeshColorTex),
			IDX(gfx::SamplerState::POINT_CLAMP),
		};

		auto cbvBufDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(MeshCB))
			.ViewFlags(gfx::BufferViewFlags::CBV)
			.Usage(gfx::ResourceUsage::DYNAMIC);

		m_MeshCbvBuf = m_GraphicsContext.CreateBuffer(cbvBufDesc, &m_MeshCB, sizeof(MeshCB));
	}

	~Hello3D()
	{
		m_GraphicsContext.DestroyPipeline(m_MeshPso);
		m_GraphicsContext.DestroyBuffer(m_MeshVtxBuf);
		m_GraphicsContext.DestroyBuffer(m_MeshCbvBuf);
		m_GraphicsContext.DestroyTexture(m_MeshColorTex);
	}

	void Update() override
	{
		static float rotation = 0.0f;
		rotation += 0.001f;
		m_MeshCB.model = float4x4::rotation_y(rotation);
	}

	void Draw() override
	{
		m_GraphicsContext.UpdateBuffer(m_MeshCbvBuf, &m_MeshCB, sizeof(MeshCB));

		SetRenderTargets();

		m_GraphicsContext.BeginRenderPass(m_MeshPso);
		{
			if (m_GraphicsContext.GetIsReady(m_MeshVtxBuf) && m_GraphicsContext.GetIsReady(m_MeshColorTex))
			{
				m_GraphicsContext.SetShaderResource(m_MeshCbvBuf, m_MeshCbvBufProxy);
				m_GraphicsContext.Draw(36);
			}
		}
		m_GraphicsContext.EndRenderPass();

		DrawToBackBuffer();
	}
};