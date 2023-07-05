#pragma once

#include "shaders_shared.h"

using namespace vast::gfx;

class Hello3D final : public SampleBase
{
private:
	TextureHandle m_DepthRT;

	PipelineHandle m_MeshPso;
	ShaderResourceProxy m_MeshCbvBufProxy;

	BufferHandle m_MeshVtxBuf;
	BufferHandle m_MeshCbvBuf;
	TextureHandle m_MeshColorTex;
	MeshCB m_MeshCB;

public:
	Hello3D(GraphicsContext& ctx) : SampleBase(ctx)
	{
		// TODO: Ideally we'd subscribe the base class and that would invoke the derived class... likely not possible.
		VAST_SUBSCRIBE_TO_EVENT("hello3d", WindowResizeEvent, VAST_EVENT_HANDLER_CB(Hello3D::OnWindowResizeEvent, WindowResizeEvent));

		auto windowSize = m_GraphicsContext.GetOutputRenderTargetSize();
		m_DepthRT = m_GraphicsContext.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, windowSize));

		m_MeshPso = m_GraphicsContext.CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "mesh.hlsl", .entryPoint = "VS_Main"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "mesh.hlsl", .entryPoint = "PS_Main"},
			.renderPassLayout = {
				.rtFormats = { m_GraphicsContext.GetOutputRenderTargetFormat() },
				.dsFormat = { ctx.GetTextureFormat(m_DepthRT) },
			},
		});
		m_MeshCbvBufProxy = m_GraphicsContext.LookupShaderResource(m_MeshPso, "CB");

		Array<Vtx3fPos3fNormal2fUv, 36> cubeVertexData =
		{ {
			// Top
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 0.0f }},
			// Bottom
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
			// Left
			{{-1.0f,-1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
			// Front
			{{-1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
			// Right
			{{ 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
			// Back
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }},
		} };

		BufferDesc vtxBufDesc =
		{
			.size	= sizeof(cubeVertexData),
			.stride = sizeof(cubeVertexData[0]),
			.viewFlags = BufViewFlags::SRV,
			.cpuAccess = BufCpuAccess::NONE,
			.isRawAccess = true,
		};
		m_MeshVtxBuf = m_GraphicsContext.CreateBuffer(vtxBufDesc, &cubeVertexData, sizeof(cubeVertexData));

		m_MeshColorTex = m_GraphicsContext.CreateTexture("image.tga");

		float fieldOfView = float(PI) / 4.0f;
		float aspectRatio = (float)windowSize.x / (float)windowSize.y;
		m_MeshCB =
		{
			.model = float4x4(),
			.view = float4x4::look_at({ -3.0f, 3.0f, -8.0f }, float3(0), float3(0, 1, 0)),
			.proj = float4x4::perspective(hlslpp::projection(hlslpp::frustum::field_of_view_x(fieldOfView, aspectRatio, 0.001f, 1000.0f), hlslpp::zclip::t::zero)),
			.cameraPos = { -3.0f, 3.0f, -8.0f },
			.vtxBufIdx = ctx.GetBindlessIndex(m_MeshVtxBuf),
			.colorTexIdx = ctx.GetBindlessIndex(m_MeshColorTex),
			.colorSamplerIdx = IDX(SamplerState::POINT_CLAMP),
		};

		BufferDesc cbvBufDesc =
		{
			.size = sizeof(MeshCB),
			.viewFlags = BufViewFlags::CBV,
			.cpuAccess = BufCpuAccess::WRITE,
			.usage = ResourceUsage::DYNAMIC,
		};
		m_MeshCbvBuf = m_GraphicsContext.CreateBuffer(cbvBufDesc, &m_MeshCB, sizeof(MeshCB));
	}

	~Hello3D()
	{
		VAST_UNSUBSCRIBE_FROM_EVENT("hello3d", WindowResizeEvent);

		m_GraphicsContext.DestroyTexture(m_DepthRT);
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

		const RenderTargetDesc outputTargetDesc = {.h = m_GraphicsContext.GetOutputRenderTarget(), .loadOp = LoadOp::CLEAR };
		const RenderTargetDesc depthTargetDesc = {.h = m_DepthRT, .loadOp = LoadOp::CLEAR, .storeOp = StoreOp::DISCARD };

		m_GraphicsContext.BeginRenderPass(m_MeshPso, RenderPassTargets{.rt = { outputTargetDesc }, .ds = depthTargetDesc });
		{
			if (m_GraphicsContext.GetIsReady(m_MeshVtxBuf) && m_GraphicsContext.GetIsReady(m_MeshColorTex))
			{
				m_GraphicsContext.SetShaderResource(m_MeshCbvBuf, m_MeshCbvBufProxy);
				m_GraphicsContext.Draw(36);
			}
		}
		m_GraphicsContext.EndRenderPass();
	}

	void OnWindowResizeEvent(const WindowResizeEvent& event) override
	{
		m_GraphicsContext.FlushGPU();
		m_GraphicsContext.DestroyTexture(m_DepthRT);
		m_DepthRT = m_GraphicsContext.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, event.m_WindowSize));


		float fieldOfView = float(PI) / 4.0f;
		float aspectRatio = (float)event.m_WindowSize.x / (float)event.m_WindowSize.y;
		m_MeshCB.proj = float4x4::perspective(hlslpp::projection(hlslpp::frustum::field_of_view_x(fieldOfView, aspectRatio, 0.001f, 1000.0f), hlslpp::zclip::t::zero));
	}
};