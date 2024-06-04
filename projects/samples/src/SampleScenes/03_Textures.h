#pragma once

#include "ISample.h"
#include "Rendering/Camera.h"
#include "Rendering/Shapes.h"
#include "Rendering/Skybox.h"
#include "Rendering/Imgui.h"

using namespace vast::gfx;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Textures
 * --------
 * 
 * Topics:
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
class Textures final : public ISample
{
private:
	PipelineHandle m_FullscreenPso;
	TextureHandle m_ColorRT;
	TextureHandle m_DepthRT;

	struct Drawable
	{
		BufferHandle vtxBuf;
		BufferHandle idxBuf;
		BufferHandle cbvBuf;
		TextureHandle colorTex;
		uint32 numIndices;

		struct CB
		{
			float4x4 modelMatrix;
			uint32 vtxBufIdx;
			uint32 colorTexIdx;
			uint32 colorSamplerIdx;
		} cb;
	};
	Array<Drawable, 2> m_TexturedDrawables;

	struct FrameCB
	{
		float4x4 viewProjMatrix;
		s_float3 cameraPos;
		uint32 skyboxTexIdx;
	} m_FrameCB;

	BufferHandle m_FrameCbvBuf;

	PipelineHandle m_TexturedMeshPso;
	ShaderResourceProxy m_TexturedMeshCbvProxy;
	ShaderResourceProxy m_FrameCbvProxy;

	Ptr<PerspectiveCamera> m_Camera;
	Ptr<Skybox> m_Skybox;
	TextureHandle m_EnvironmentCubeTex;

public:
	Textures(GraphicsContext& ctx_) : ISample(ctx_)
	{	
		// Create full-screen pass PSO
		m_FullscreenPso = ctx.CreatePipeline(PipelineDesc{
			.vs = AllocVertexShaderDesc("Fullscreen.hlsl"),
			.ps = AllocPixelShaderDesc("Fullscreen.hlsl"),
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
			});

		// Create full-screen color and depth intermediate buffers to render our meshes to.
		uint2 backBufferSize = ctx.GetBackBufferSize();
		float4 clearColor = float4(0.6f, 0.2f, 0.3f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, backBufferSize, clearColor));
		m_DepthRT = ctx.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, backBufferSize));

		// Create perspective camera
		float3 cameraPos = float3(0.0f, 1.0f, -5.0f);
		m_Camera = MakePtr<PerspectiveCamera>(cameraPos, float3(0, 0, 0), float3(0, 1, 0), ctx.GetBackBufferAspectRatio(), DEG_TO_RAD(45.0f));

		// Create skybox object
		m_Skybox = MakePtr<Skybox>(ctx, ctx.GetTextureFormat(m_ColorRT), ctx.GetTextureFormat(m_DepthRT));

		// Load a cube texture from file to be used in the skybox.
		m_EnvironmentCubeTex = ctx.LoadTextureFromFile("yokohama2_cube1024.dds");

		m_FrameCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();
		m_FrameCB.cameraPos = s_float3(cameraPos.x, cameraPos.y, cameraPos.z);
		m_FrameCB.skyboxTexIdx = ctx.GetBindlessSRV(m_EnvironmentCubeTex);
		m_FrameCbvBuf = ctx.CreateBuffer(AllocCbvBufferDesc(sizeof(FrameCB)), &m_FrameCB, sizeof(FrameCB));

		PipelineDesc pipelineDesc =
		{
			.vs = AllocVertexShaderDesc("03_TexturedMesh.hlsl", GetSamplesShaderSourcePath()),
			.ps = AllocPixelShaderDesc("03_TexturedMesh.hlsl", GetSamplesShaderSourcePath()),
			.renderPassLayout =
			{
				.rtFormats = { ctx.GetTextureFormat(m_ColorRT)  },
				.dsFormat = { ctx.GetTextureFormat(m_DepthRT) },
			},
		};
		m_TexturedMeshPso = ctx.CreatePipeline(pipelineDesc);

		m_TexturedMeshCbvProxy = ctx.LookupShaderResource(m_TexturedMeshPso, "ObjectConstantBuffer");
		m_FrameCbvProxy = ctx.LookupShaderResource(m_TexturedMeshPso, "FrameConstantBuffer");

		{
			// Create the cube vertex buffer with bindless access.
			auto vtxBufDesc = AllocVertexBufferDesc(sizeof(Cube::s_Vertices_PosNormalUv), sizeof(Cube::s_Vertices_PosNormalUv[0]));
			auto cbvBufDesc = AllocCbvBufferDesc(sizeof(Drawable::CB));

			m_TexturedDrawables[0].vtxBuf = ctx.CreateBuffer(vtxBufDesc, &Cube::s_Vertices_PosNormalUv, sizeof(Cube::s_Vertices_PosNormalUv));
			// Note: This time, we render the cube without an index buffer.
			m_TexturedDrawables[0].numIndices = static_cast<uint32>(Cube::s_Vertices_PosNormalUv.size());

			m_TexturedDrawables[0].colorTex = ctx.LoadTextureFromFile("image.tga");

			m_TexturedDrawables[0].cb.modelMatrix = float4x4::translation(1.5f, 0.0f, 0.0f);
			m_TexturedDrawables[0].cb.vtxBufIdx = ctx.GetBindlessSRV(m_TexturedDrawables[0].vtxBuf);
			m_TexturedDrawables[0].cb.colorTexIdx = ctx.GetBindlessSRV(m_TexturedDrawables[0].colorTex);
			m_TexturedDrawables[0].cb.colorSamplerIdx = IDX(SamplerState::POINT_CLAMP);

			m_TexturedDrawables[0].cbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_TexturedDrawables[0].cb, sizeof(Drawable::CB));
		}

		{
			Vector<Vtx3fPos3fNormal2fUv> sphereVertexData;
			Vector<uint16> sphereIndexData;
			Sphere::ConstructUVSphere(1.5f, 18, 36, sphereVertexData, sphereIndexData);

			const uint32 vtxSize = static_cast<uint32>(sphereVertexData.size() * sizeof(Vtx3fPos3fNormal2fUv));
			const uint32 numIndices = static_cast<uint32>(sphereIndexData.size());

			auto vtxBufDesc = AllocVertexBufferDesc(vtxSize, sizeof(Vtx3fPos3fNormal2fUv));
			auto idxBufDesc = AllocIndexBufferDesc(numIndices);
			auto cbvBufDesc = AllocCbvBufferDesc(sizeof(Drawable::CB));

			m_TexturedDrawables[1].vtxBuf = ctx.CreateBuffer(vtxBufDesc, sphereVertexData.data(), sphereVertexData.size() * sizeof(Vtx3fPos3fNormal2fUv));
			m_TexturedDrawables[1].idxBuf = ctx.CreateBuffer(idxBufDesc, sphereIndexData.data(), sphereIndexData.size() * sizeof(uint16));
			m_TexturedDrawables[1].numIndices = numIndices;

			m_TexturedDrawables[1].colorTex = ctx.LoadTextureFromFile("2k_earth_daymap.jpg");

			m_TexturedDrawables[1].cb.modelMatrix = float4x4::translation(-1.5f, 0.0f, 0.0f);
			m_TexturedDrawables[1].cb.vtxBufIdx = ctx.GetBindlessSRV(m_TexturedDrawables[1].vtxBuf);
			m_TexturedDrawables[1].cb.colorTexIdx = ctx.GetBindlessSRV(m_TexturedDrawables[1].colorTex);
			m_TexturedDrawables[1].cb.colorSamplerIdx = IDX(SamplerState::LINEAR_CLAMP);

			m_TexturedDrawables[1].cbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_TexturedDrawables[1].cb, sizeof(Drawable::CB));
		}

		// TODO: Ideally we'd subscribe the base class and that would invoke the derived class... likely not possible.
		VAST_SUBSCRIBE_TO_EVENT("Textures", WindowResizeEvent, VAST_EVENT_HANDLER_CB(Textures::OnWindowResizeEvent, WindowResizeEvent));
		VAST_SUBSCRIBE_TO_EVENT("Textures", ReloadShadersEvent, VAST_EVENT_HANDLER_CB(Textures::OnReloadShadersEvent));
	}

	~Textures()
	{
		// Clean up GPU resources created for this sample.
		ctx.DestroyPipeline(m_FullscreenPso);
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT);
		ctx.DestroyTexture(m_EnvironmentCubeTex);

		ctx.DestroyBuffer(m_FrameCbvBuf);
		ctx.DestroyPipeline(m_TexturedMeshPso);

		for (auto& i : m_TexturedDrawables)
		{
			ctx.DestroyBuffer(i.vtxBuf);
			if (i.idxBuf.IsValid()) ctx.DestroyBuffer(i.idxBuf);
			ctx.DestroyBuffer(i.cbvBuf);
			ctx.DestroyTexture(i.colorTex);
		}

		m_Skybox = nullptr;

		VAST_UNSUBSCRIBE_FROM_EVENT("Textures", WindowResizeEvent);
		VAST_UNSUBSCRIBE_FROM_EVENT("Textures", ReloadShadersEvent);
	}

	void Update() override
	{
		m_TexturedDrawables[0].cb.modelMatrix = hlslpp::mul(float4x4::rotation_y(0.001f), m_TexturedDrawables[0].cb.modelMatrix);
		m_TexturedDrawables[1].cb.modelMatrix = hlslpp::mul(float4x4::rotation_y(0.001f), m_TexturedDrawables[1].cb.modelMatrix);
	}

	void Render() override
	{
		for (auto& i : m_TexturedDrawables)
		{
			ctx.UpdateBuffer(i.cbvBuf, &i.cb, sizeof(Drawable::CB));
		}

		// Transition and clear our intermediate color and depth targets and set the pipeline state
		// to render the cube.
		// TODO: NextUsage causes a crash when resizing the window multiple times.
		RenderTargetDesc forwardRtDesc = { .h = m_ColorRT, .loadOp = LoadOp::CLEAR/*, .nextUsage = ResourceState::RENDER_TARGET*/ };
		RenderTargetDesc forwardDsDesc = { .h = m_DepthRT, .loadOp = LoadOp::CLEAR/*, .nextUsage = ResourceState::DEPTH_WRITE */};
		ctx.BeginRenderPass(m_TexturedMeshPso, RenderPassTargets{.rt = { forwardRtDesc }, .ds = forwardDsDesc });
		{
			ctx.BindConstantBuffer(m_FrameCbvProxy, m_FrameCbvBuf);

			for (auto& i : m_TexturedDrawables)
			{
				if (ctx.GetIsReady(i.vtxBuf) && ctx.GetIsReady(i.colorTex))
				{
					if (i.idxBuf.IsValid())
					{
						if (!ctx.GetIsReady(i.idxBuf))
							continue;

						ctx.BindConstantBuffer(m_TexturedMeshCbvProxy, i.cbvBuf);
						ctx.BindIndexBuffer(i.idxBuf);
						ctx.DrawIndexed(i.numIndices);
					}
					else
					{
						ctx.BindConstantBuffer(m_TexturedMeshCbvProxy, i.cbvBuf);
						ctx.Draw(i.numIndices);
					}
				}
			}
		}
		ctx.EndRenderPass();

		RenderTargetDesc skyboxRtDesc = { .h = m_ColorRT };
		RenderTargetDesc skyboxDsDesc = { .h = m_DepthRT, .storeOp = StoreOp::DISCARD };
		m_Skybox->Render(m_EnvironmentCubeTex, skyboxRtDesc, skyboxDsDesc, *m_Camera);

		// Render our color target to the back buffer and gamma correct it.
		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD, StoreOp::STORE);
		{
			uint32 srvIndex = ctx.GetBindlessSRV(m_ColorRT);
			ctx.SetPushConstants(&srvIndex, sizeof(uint32));
			ctx.DrawFullscreenTriangle();
		}
		ctx.EndRenderPass();
	}

	void OnWindowResizeEvent(const WindowResizeEvent& event) override
	{
		// If the window is resized we need to update the size of the render targets, as well as our camera aspect ratio.
		ctx.FlushGPU();
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT);
		float4 clearColor = float4(0.6f, 0.2f, 0.3f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, event.m_WindowSize, clearColor));
		m_DepthRT = ctx.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, event.m_WindowSize));
	}

	void OnReloadShadersEvent() override
	{
		ctx.ReloadShaders(m_FullscreenPso);
		ctx.ReloadShaders(m_TexturedMeshPso);
	}

};