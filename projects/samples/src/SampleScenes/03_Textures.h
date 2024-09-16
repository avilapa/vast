#pragma once

#include "ISample.h"
#include "Rendering/Camera.h"
#include "Rendering/Shapes.h"
#include "Rendering/Skybox.h"
#include "Rendering/Imgui.h"

using namespace vast;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Textures
 * --------
 * This sample renders a textured cube and sphere rotating inside a skybox and reflecting the
 * environment. The textures are loaded from disk. The cube uses point sampling while the sphere
 * does bilinear filtering.
 * 
 * All code for this sample is contained within this file plus the shader files 'Fullscreen.hlsl'
 * and '03_TexturedMesh.hlsl'.
 * 
 * Topics: texture loading, samplers, perspective camera, skybox
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
	Textures(GraphicsContext& ctx) : ISample(ctx)
	{	
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		// Create full-screen pass PSO
		m_FullscreenPso = rm.CreatePipeline(PipelineDesc
		{
			.vs = AllocVertexShaderDesc("Fullscreen.hlsl"),
			.ps = AllocPixelShaderDesc("Fullscreen.hlsl"),
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
		});

		// Create full-screen color and depth intermediate buffers to render our meshes to.
		CreateIntermediateRenderTargets(ctx.GetBackBufferSize());

		// Create perspective camera
		float3 cameraPos = float3(0.0f, 1.0f, -5.0f);
		m_Camera = MakePtr<PerspectiveCamera>(cameraPos, float3(0, 0, 0), float3(0, 1, 0), ctx.GetBackBufferAspectRatio(), DEG_TO_RAD(45.0f));

		// Create skybox object
		m_Skybox = MakePtr<Skybox>(ctx, rm.GetTextureFormat(m_ColorRT), rm.GetTextureFormat(m_DepthRT));

		// Load a cube texture from file to be used in the skybox.
		m_EnvironmentCubeTex = rm.LoadTextureFromFile("yokohama2_cube1024.dds");

		m_FrameCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();
		m_FrameCB.cameraPos = s_float3(cameraPos.x, cameraPos.y, cameraPos.z);
		m_FrameCB.skyboxTexIdx = rm.GetBindlessSRV(m_EnvironmentCubeTex);
		m_FrameCbvBuf = rm.CreateBuffer(AllocCbvBufferDesc(sizeof(FrameCB)), &m_FrameCB, sizeof(FrameCB), "Frame Constant Buffer");

		PipelineDesc pipelineDesc =
		{
			.vs = AllocVertexShaderDesc("03_TexturedMesh.hlsl", m_SamplesShaderSourcePath.c_str()),
			.ps = AllocPixelShaderDesc("03_TexturedMesh.hlsl", m_SamplesShaderSourcePath.c_str()),
			.renderPassLayout =
			{
				.rtFormats = { rm.GetTextureFormat(m_ColorRT)  },
				.dsFormat = { rm.GetTextureFormat(m_DepthRT) },
			},
		};
		m_TexturedMeshPso = rm.CreatePipeline(pipelineDesc);

		m_TexturedMeshCbvProxy = rm.LookupShaderResource(m_TexturedMeshPso, "ObjectConstantBuffer");
		m_FrameCbvProxy = rm.LookupShaderResource(m_TexturedMeshPso, "FrameConstantBuffer");

		{
			// Create the cube vertex buffer with bindless access.
			auto vtxBufDesc = AllocVertexBufferDesc(sizeof(Cube::s_Vertices_PosNormalUv), sizeof(Cube::s_Vertices_PosNormalUv[0]));
			auto cbvBufDesc = AllocCbvBufferDesc(sizeof(Drawable::CB));

			m_TexturedDrawables[0].vtxBuf = rm.CreateBuffer(vtxBufDesc, &Cube::s_Vertices_PosNormalUv, sizeof(Cube::s_Vertices_PosNormalUv), "Cube Vertex Buffer");
			// Note: This time, we render the cube without an index buffer.
			m_TexturedDrawables[0].numIndices = static_cast<uint32>(Cube::s_Vertices_PosNormalUv.size());

			m_TexturedDrawables[0].colorTex = rm.LoadTextureFromFile("image.tga");

			m_TexturedDrawables[0].cb.modelMatrix = float4x4::translation(1.5f, 0.0f, 0.0f);
			m_TexturedDrawables[0].cb.vtxBufIdx = rm.GetBindlessSRV(m_TexturedDrawables[0].vtxBuf);
			m_TexturedDrawables[0].cb.colorTexIdx = rm.GetBindlessSRV(m_TexturedDrawables[0].colorTex);
			m_TexturedDrawables[0].cb.colorSamplerIdx = IDX(SamplerState::POINT_CLAMP);

			m_TexturedDrawables[0].cbvBuf = rm.CreateBuffer(cbvBufDesc, &m_TexturedDrawables[0].cb, sizeof(Drawable::CB), "Cube Constant Buffer");
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

			m_TexturedDrawables[1].vtxBuf = rm.CreateBuffer(vtxBufDesc, sphereVertexData.data(), sphereVertexData.size() * sizeof(Vtx3fPos3fNormal2fUv), "Sphere Vertex Buffer");
			m_TexturedDrawables[1].idxBuf = rm.CreateBuffer(idxBufDesc, sphereIndexData.data(), sphereIndexData.size() * sizeof(uint16), "Sphere Index Buffer");
			m_TexturedDrawables[1].numIndices = numIndices;

			m_TexturedDrawables[1].colorTex = rm.LoadTextureFromFile("2k_earth_daymap.jpg");

			m_TexturedDrawables[1].cb.modelMatrix = float4x4::translation(-1.5f, 0.0f, 0.0f);
			m_TexturedDrawables[1].cb.vtxBufIdx = rm.GetBindlessSRV(m_TexturedDrawables[1].vtxBuf);
			m_TexturedDrawables[1].cb.colorTexIdx = rm.GetBindlessSRV(m_TexturedDrawables[1].colorTex);
			m_TexturedDrawables[1].cb.colorSamplerIdx = IDX(SamplerState::LINEAR_CLAMP);

			m_TexturedDrawables[1].cbvBuf = rm.CreateBuffer(cbvBufDesc, &m_TexturedDrawables[1].cb, sizeof(Drawable::CB), "Sphere Constant Buffer");
		}
	}

	~Textures()
	{
		// Clean up GPU resources created for this sample.
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyPipeline(m_FullscreenPso);
		rm.DestroyTexture(m_EnvironmentCubeTex);

		DestroyIntermediateRenderTargets();

		rm.DestroyBuffer(m_FrameCbvBuf);
		rm.DestroyPipeline(m_TexturedMeshPso);

		for (auto& i : m_TexturedDrawables)
		{
			rm.DestroyBuffer(i.vtxBuf);
			if (i.idxBuf.IsValid()) rm.DestroyBuffer(i.idxBuf);
			rm.DestroyBuffer(i.cbvBuf);
			rm.DestroyTexture(i.colorTex);
		}

		m_Skybox = nullptr;
	}

	void Update(float dt) override
	{
		m_TexturedDrawables[0].cb.modelMatrix = hlslpp::mul(float4x4::rotation_y(0.001f * dt), m_TexturedDrawables[0].cb.modelMatrix);
		m_TexturedDrawables[1].cb.modelMatrix = hlslpp::mul(float4x4::rotation_y(0.001f * dt), m_TexturedDrawables[1].cb.modelMatrix);
	}

	void Render() override
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.UpdateBuffer(m_FrameCbvBuf, &m_FrameCB, sizeof(FrameCB));

		for (auto& i : m_TexturedDrawables)
		{
			rm.UpdateBuffer(i.cbvBuf, &i.cb, sizeof(Drawable::CB));
		}

		VAST_PROFILE_GPU_BEGIN("Forward Render Pass", ctx);

		// Transition and clear our intermediate color and depth targets and set the pipeline state
		// to render the cube.
		const RenderPassDesc forwardPassDesc =
		{
			.rt = { RenderTargetDesc{.h = m_ColorRT, .loadOp = LoadOp::CLEAR, .nextUsage = ResourceState::RENDER_TARGET } },
			.ds = {.h = m_DepthRT, .loadOp = LoadOp::CLEAR, .nextUsage = ResourceState::DEPTH_WRITE }
		};

		ctx.BeginRenderPass(m_TexturedMeshPso, forwardPassDesc);
		{
			ctx.BindConstantBuffer(m_FrameCbvProxy, m_FrameCbvBuf);

			for (auto& i : m_TexturedDrawables)
			{
				if (rm.GetIsReady(i.vtxBuf) && rm.GetIsReady(i.colorTex))
				{
					if (i.idxBuf.IsValid())
					{
						if (!rm.GetIsReady(i.idxBuf))
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

		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("Skybox Render Pass", ctx);

		RenderTargetDesc skyboxRtDesc = { .h = m_ColorRT };
		RenderTargetDesc skyboxDsDesc = { .h = m_DepthRT, .storeOp = StoreOp::DISCARD };

		m_Skybox->Render(m_EnvironmentCubeTex, skyboxRtDesc, skyboxDsDesc, *m_Camera);

		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("BackBuffer Pass", ctx);

		// Render our color target to the back buffer and gamma correct it.
		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD);
		{
			uint32 srvIndex = rm.GetBindlessSRV(m_ColorRT);
			ctx.SetPushConstants(&srvIndex, sizeof(uint32));
			ctx.DrawFullscreenTriangle();
		}
		ctx.EndRenderPass();

		VAST_PROFILE_GPU_END();
	}

	void OnWindowResizeEvent(const WindowResizeEvent& event) override
	{
		// If the window is resized we need to update the size of the render targets, as well as our camera aspect ratio.
		ctx.FlushGPU();

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		DestroyIntermediateRenderTargets();
		CreateIntermediateRenderTargets(event.m_WindowSize);

		m_Camera->SetAspectRatio(ctx.GetBackBufferAspectRatio());
		m_FrameCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();

	}

	void OnReloadShadersEvent() override
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.ReloadShaders(m_FullscreenPso);
		rm.ReloadShaders(m_TexturedMeshPso);
	}

	void CreateIntermediateRenderTargets(uint2 dimensions)
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		m_ColorRT = rm.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, dimensions), nullptr, "Color RT");
		m_DepthRT = rm.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, dimensions), nullptr, "Depth RT");
// 		m_ColorRTIdx = rm.GetBindlessSRV(m_ColorRT);
	}

	void DestroyIntermediateRenderTargets()
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyTexture(m_ColorRT);
		rm.DestroyTexture(m_DepthRT);
	}

};