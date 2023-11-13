#pragma once

#include "ISample.h"
#include "Rendering/Camera.h"
#include "Rendering/Imgui.h"

using namespace vast::gfx;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Instancing
 * ----------
 * This sample uses instancing to efficiently draw 50000 moving cubes to the screen. The cubes this
 * time are rendered using an index buffer, so the vertex buffer is much smaller. A CBV holds the
 * vertex buffer bindless index and the camera matrix, while a structured buffer holds per instance
 * data such as the model matrix and a random color for each cube. This scene showcases precision
 * problems typical of a standard depth buffer implementation, which are easily fixed by using a
 * reverse-z depth buffer. We implement both methods and a simple user interface allows us to allow
 * switching between the two methods to observe the differences.
 * 
 * All code for this sample is contained within this file plus the shader files 'instancing.hlsl'
 * and 'fullscreen.hlsl' containing code for rendering the cube and gamma correcting the color
 * result to the back buffer respectively.
 * 
 * Topics: instancing, structured buffer, index buffer, reverse-z depth buffer, camera object
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static Array<s_float3, 8> s_CubeVertexData =
{ {
	{-1.0f,  1.0f,  1.0f },
	{ 1.0f,  1.0f,  1.0f },
	{-1.0f, -1.0f,  1.0f },
	{ 1.0f, -1.0f,  1.0f },
	{-1.0f,  1.0f, -1.0f },
	{ 1.0f,  1.0f, -1.0f },
	{-1.0f, -1.0f, -1.0f },
	{ 1.0f, -1.0f, -1.0f },
} };

static Array<uint16, 36> s_CubeIndexData =
{ {
	0, 1, 2,
	1, 3, 2,
	4, 6, 5,
	5, 6, 7,
	0, 2, 4,
	4, 2, 6,
	1, 5, 3,
	5, 7, 3,
	0, 4, 1,
	4, 5, 1,
	2, 3, 6,
	6, 3, 7,
} };

class Instancing final : public ISample
{
private:
	enum DepthBufferMode
	{
		STANDARD = 0,
		REVERSE_Z,
		COUNT,
	};

	Ptr<PerspectiveCamera> m_Camera;

	PipelineHandle m_FullscreenPso;
	TextureHandle m_ColorRT;
	Array<TextureHandle, DepthBufferMode::COUNT> m_DepthRT;
	Array<PipelineHandle, DepthBufferMode::COUNT> m_CubeInstPso;
	BufferHandle m_CubeInstBuf; ShaderResourceProxy m_InstBufProxy;
	BufferHandle m_CubeCbvBuf; ShaderResourceProxy m_CbvBufProxy;
	BufferHandle m_CubeVtxBuf;
	BufferHandle m_CubeIdxBuf;

	struct CubeCB
	{
		float4x4 viewProjMatrix;
		uint32 vtxBufIdx;
	} m_CubeCB;

	struct InstanceData
	{
		float4x4 modelMatrix;
		s_float3 color;
		float __pad0;
	};

	static const uint32 s_NumInstances = 50000;
	Array<InstanceData, s_NumInstances> m_InstanceData;

	bool m_bViewChanged = false;
	bool m_bDepthUseReverseZ = true;

public:
	Instancing(GraphicsContext& ctx_) : ISample(ctx_)
	{
		const float3 cameraPos = float3(0.0f, 30.0f, -20.0f);
		const float3 lookAt = float3(0.0f, 0.0f, 100.0f);
		m_Camera = MakePtr<PerspectiveCamera>(cameraPos, lookAt, float3(0, 1, 0), 
			ctx.GetBackBufferAspectRatio(), DEG_TO_RAD(45.0f), 0.001f, 10000.0f, m_bDepthUseReverseZ);

		m_FullscreenPso = ctx.CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "fullscreen.hlsl", .entryPoint = "VS_Main"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "fullscreen.hlsl", .entryPoint = "PS_Main"},
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
		});

		uint2 backBufferSize = ctx.GetBackBufferSize();
		float4 clearColor = float4(0.02f, 0.02f, 0.02f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, backBufferSize, clearColor));

		// Create both standard and reverse-z depth buffers, the only difference being the inverted
		// depth clear value.
		auto dsDesc = AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, backBufferSize);
		dsDesc.clear.ds.depth = CLEAR_DEPTH_VALUE_STANDARD;
		m_DepthRT[DepthBufferMode::STANDARD] = ctx.CreateTexture(dsDesc);
		dsDesc.clear.ds.depth = CLEAR_DEPTH_VALUE_REVERSE_Z;
		m_DepthRT[DepthBufferMode::REVERSE_Z] = ctx.CreateTexture(dsDesc);

		// Create both standard and reverse-z PSOs with different Depth Stencil States.
		PipelineDesc psoDesc =
		{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "instancing.hlsl", .entryPoint = "VS_Cube"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "instancing.hlsl", .entryPoint = "PS_Cube"},
			.renderPassLayout =
			{
				.rtFormats = { ctx.GetTextureFormat(m_ColorRT) },
				.dsFormat = { ctx.GetTextureFormat(m_DepthRT[0]) },
			},
		};
		psoDesc.depthStencilState.depthFunc = CompareFunc::LESS_EQUAL;
		m_CubeInstPso[DepthBufferMode::STANDARD] = ctx.CreatePipeline(psoDesc);
		psoDesc.depthStencilState.depthFunc = CompareFunc::GREATER_EQUAL;
		m_CubeInstPso[DepthBufferMode::REVERSE_Z] = ctx.CreatePipeline(psoDesc);

		// Locate the instance buffer and constant buffer slots in the shader.
		m_InstBufProxy = ctx.LookupShaderResource(m_CubeInstPso[0], "InstanceBuffer");
		m_CbvBufProxy = ctx.LookupShaderResource(m_CubeInstPso[0], "ObjectConstantBuffer");

		auto vtxBufDesc = AllocVertexBufferDesc(sizeof(s_CubeVertexData), sizeof(s_CubeVertexData[0]));
		m_CubeVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_CubeVertexData, sizeof(s_CubeVertexData));

		// Create index buffer.
		auto idxBufDesc = AllocIndexBufferDesc(static_cast<uint32>(s_CubeIndexData.size()));
		m_CubeIdxBuf = ctx.CreateBuffer(idxBufDesc, s_CubeIndexData.data(), s_CubeIndexData.size() * sizeof(s_CubeIndexData[0]));

		// Create a structured buffer to hold our instance data and initialize it.
		auto instBufDesc = AllocStructuredBufferDesc(sizeof(InstanceData) * s_NumInstances, sizeof(InstanceData), ResourceUsage::UPLOAD);
		m_CubeInstBuf = ctx.CreateBuffer(instBufDesc);

		for (auto& i : m_InstanceData)
		{
			i.modelMatrix = float4x4::identity();
			i.color = { rand() % 255 / 255.0f, rand() % 255 / 255.0f, rand() % 255 / 255.0f };
		}

		m_CubeCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();
		m_CubeCB.vtxBufIdx = ctx.GetBindlessSRV(m_CubeVtxBuf);
		m_CubeCbvBuf = ctx.CreateBuffer(AllocCbvBufferDesc(sizeof(CubeCB)), &m_CubeCB, sizeof(CubeCB));

		// TODO: Ideally we'd subscribe the base class and that would invoke the derived class... likely not possible.
		VAST_SUBSCRIBE_TO_EVENT("Instancing", WindowResizeEvent, VAST_EVENT_HANDLER_CB(Instancing::OnWindowResizeEvent, WindowResizeEvent));
		VAST_SUBSCRIBE_TO_EVENT("Instancing", ReloadShadersEvent, VAST_EVENT_HANDLER_CB(Instancing::OnReloadShadersEvent));
	}

	~Instancing()
	{
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT[DepthBufferMode::STANDARD]);
		ctx.DestroyTexture(m_DepthRT[DepthBufferMode::REVERSE_Z]);
		ctx.DestroyPipeline(m_FullscreenPso);
		ctx.DestroyPipeline(m_CubeInstPso[DepthBufferMode::STANDARD]);
		ctx.DestroyPipeline(m_CubeInstPso[DepthBufferMode::REVERSE_Z]);
		ctx.DestroyBuffer(m_CubeInstBuf);
		ctx.DestroyBuffer(m_CubeCbvBuf);
		ctx.DestroyBuffer(m_CubeVtxBuf);
		ctx.DestroyBuffer(m_CubeIdxBuf);

		VAST_UNSUBSCRIBE_FROM_EVENT("Instancing", WindowResizeEvent);
		VAST_UNSUBSCRIBE_FROM_EVENT("Instancing", ReloadShadersEvent);
	}

	void Update() override
	{
		// Update the transformation matrices for all cube instances.
		static float v = 0.0f;
		v += 0.01f;

		const uint32 numCubesPerRow = 100;
		const float spacingX = 2.5f;
		const float spacingZ = 5.0f;
		const float maxHeightY = 7.5f;
		for (uint32 i = 0; i < s_NumInstances; ++i)
		{
			float x = float(i % numCubesPerRow) * spacingX - float(numCubesPerRow / 2) * spacingX;
			float y = maxHeightY * float(sin((i/100) * PI / 10 + v));
			float z = float(i / numCubesPerRow) * spacingZ;
			m_InstanceData[i].modelMatrix = float4x4::translation(float3(x, y, z));
		}
	}

	void Render() override
	{
		VAST_PROFILE_GPU_BEGIN("Update Buffers", &ctx);
		if (m_bViewChanged)
		{
			m_CubeCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();
			ctx.UpdateBuffer(m_CubeCbvBuf, &m_CubeCB, sizeof(CubeCB));
			m_bViewChanged = false;
		}

		// Upload the instance buffer with the model matrices to render this frame.
		ctx.UpdateBuffer(m_CubeInstBuf, &m_InstanceData, sizeof(InstanceData) * s_NumInstances);
		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("Main Render Pass", &ctx);

		// Select the PSO and depth targets for the current depth buffer mode.
		auto pso = m_CubeInstPso[DepthBufferMode::STANDARD];
		auto depthRt = m_DepthRT[DepthBufferMode::STANDARD];
		if (m_bDepthUseReverseZ)
		{
			pso = m_CubeInstPso[DepthBufferMode::REVERSE_Z];
			depthRt = m_DepthRT[DepthBufferMode::REVERSE_Z];
		}

		const RenderTargetDesc colorTargetDesc = {.h = m_ColorRT, .loadOp = LoadOp::CLEAR };
		const RenderTargetDesc depthTargetDesc = {.h = depthRt, .loadOp = LoadOp::CLEAR, .storeOp = StoreOp::DISCARD };
		ctx.BeginRenderPass(pso, RenderPassTargets{.rt = { colorTargetDesc }, .ds = depthTargetDesc });
		{
			if (ctx.GetIsReady(m_CubeVtxBuf) && ctx.GetIsReady(m_CubeIdxBuf) /*&& ctx.GetIsReady(m_CubeInstBuf)*/)
			{
				// Bind our instance buffer and CBV.
				ctx.BindConstantBuffer(m_CbvBufProxy, m_CubeCbvBuf);
				ctx.BindSRV(m_InstBufProxy, m_CubeInstBuf);
				// Set the index buffer and draw all instances in a single draw call.
				ctx.BindIndexBuffer(m_CubeIdxBuf, 0, IndexBufFormat::R16_UINT);
				ctx.DrawIndexedInstanced(static_cast<uint32>(s_CubeIndexData.size()), s_NumInstances, 0, 0, 0);
			}
		}
		ctx.EndRenderPass();

		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("BackBuffer Pass", &ctx);

		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD, StoreOp::STORE);
		{
			uint32 srvIndex = ctx.GetBindlessSRV(m_ColorRT);
			ctx.SetPushConstants(&srvIndex, sizeof(uint32));
			ctx.DrawFullscreenTriangle();
		}
		ctx.EndRenderPass();

		VAST_PROFILE_GPU_END();
	}

	void OnWindowResizeEvent(const WindowResizeEvent& event) override
	{
		ctx.FlushGPU();
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT[DepthBufferMode::STANDARD]);
		ctx.DestroyTexture(m_DepthRT[DepthBufferMode::REVERSE_Z]);

		float4 clearColor = float4(0.02f, 0.02f, 0.02f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, event.m_WindowSize, clearColor));

		auto dsDesc = AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, event.m_WindowSize);
		dsDesc.clear.ds.depth = 1.0f;
		m_DepthRT[DepthBufferMode::STANDARD] = ctx.CreateTexture(dsDesc);
		dsDesc.clear.ds.depth = 0.0f;
		m_DepthRT[DepthBufferMode::REVERSE_Z] = ctx.CreateTexture(dsDesc);

		m_Camera->SetAspectRatio(ctx.GetBackBufferAspectRatio());
		m_bViewChanged = true;
	}

	void OnReloadShadersEvent() override
	{
		ctx.ReloadShaders(m_FullscreenPso);
		ctx.ReloadShaders(m_CubeInstPso[DepthBufferMode::STANDARD]);
		ctx.ReloadShaders(m_CubeInstPso[DepthBufferMode::REVERSE_Z]);
	}

	void OnGUI() override
	{
		if (ImGui::Begin("Depth Buffer Settings"))
		{
			if (ImGui::Checkbox("Use Reverse-Z", &m_bDepthUseReverseZ))
			{
				m_Camera->SetIsReverseZ(m_bDepthUseReverseZ);
				m_bViewChanged = true;
			}
		}
		ImGui::End();
	}
};