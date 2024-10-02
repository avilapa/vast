#pragma once

#include "ISample.h"
#include "Rendering/Camera.h"
#include "Rendering/Shapes.h"
#include "Rendering/Imgui.h"

using namespace vast;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Instancing
 * ----------
 * This sample uses instancing to efficiently draw 50000 moving cubes to the screen. A structured 
 * buffer holds per instance data consisting of the model matrix and a random color for each cube. 
 * This scene showcases precision problems at a distance typical of a standard depth buffer 
 * implementation, which are fixed by using a reverse-z depth buffer. We implement both methods and
 * a simple user interface to switch between the two methods to observe the differences.
 * 
 * All code for this sample is contained within this file plus the shader files 'Fullscreen.hlsl'
 * and '02_InstancedCube.hlsl'. 
 * 
 * Topics: instancing, structured buffer, reverse-z depth buffer, camera object
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
	int s_NumInstancesToRender = s_NumInstances;

	bool m_bViewChanged = false;
	bool m_bDepthUseReverseZ = true;

public:
	Instancing(GraphicsContext& ctx) : ISample(ctx)
	{
		// Create camera object
		const float3 cameraPos = float3(0.0f, 30.0f, -20.0f);
		const float3 lookAt = float3(0.0f, 0.0f, 100.0f);
		m_Camera = MakePtr<PerspectiveCamera>(cameraPos, lookAt, float3(0, 1, 0), 
			ctx.GetBackBufferAspectRatio(), DEG_TO_RAD(45.0f), 0.001f, 10000.0f, m_bDepthUseReverseZ);

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		m_FullscreenPso = rm.CreatePipeline(PipelineDesc
		{
			.vs = AllocVertexShaderDesc("Fullscreen.hlsl"),
			.ps = AllocPixelShaderDesc("Fullscreen.hlsl"),
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
		});

		uint2 backBufferSize = ctx.GetBackBufferSize();
		float4 clearColor = float4(0.02f, 0.02f, 0.02f, 1.0f);
		m_ColorRT = rm.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, backBufferSize, clearColor), nullptr, "Color RT");

		// Create both standard and reverse-z depth buffers, the only difference being the inverted
		// depth clear value.
		auto dsDesc = AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, backBufferSize);
		dsDesc.clear.ds.depth = CLEAR_DEPTH_VALUE_STANDARD;
		m_DepthRT[DepthBufferMode::STANDARD] = rm.CreateTexture(dsDesc, nullptr, "Depth RT (Standard)");
		dsDesc.clear.ds.depth = CLEAR_DEPTH_VALUE_REVERSE_Z;
		m_DepthRT[DepthBufferMode::REVERSE_Z] = rm.CreateTexture(dsDesc, nullptr, "Depth RT (Reverse-Z)");

		// Create both standard and reverse-z PSOs with different Depth Stencil States.
		PipelineDesc psoDesc =
		{
			.vs = AllocVertexShaderDesc("02_InstancedCube.hlsl", m_SamplesShaderSourcePath.c_str()),
			.ps = AllocPixelShaderDesc("02_InstancedCube.hlsl", m_SamplesShaderSourcePath.c_str()),
			.renderPassLayout =
			{
				.rtFormats = { rm.GetTextureFormat(m_ColorRT) },
				.dsFormat = { rm.GetTextureFormat(m_DepthRT[0]) },
			},
		};
		psoDesc.depthStencilState.depthFunc = CompareFunc::LESS_EQUAL;
		m_CubeInstPso[DepthBufferMode::STANDARD] = rm.CreatePipeline(psoDesc);
		psoDesc.depthStencilState.depthFunc = CompareFunc::GREATER_EQUAL;
		m_CubeInstPso[DepthBufferMode::REVERSE_Z] = rm.CreatePipeline(psoDesc);

		// Locate the instance buffer and constant buffer slots in the shader.
		m_InstBufProxy = rm.LookupShaderResource(m_CubeInstPso[0], "InstanceBuffer");
		m_CbvBufProxy = rm.LookupShaderResource(m_CubeInstPso[0], "ObjectConstantBuffer");

		// Create the cube vertex buffer with bindless access.
		auto vtxBufDesc = AllocVertexBufferDesc(sizeof(Cube::s_VerticesIndexed_Pos), sizeof(Cube::s_VerticesIndexed_Pos[0]));
		m_CubeVtxBuf = rm.CreateBuffer(vtxBufDesc, &Cube::s_VerticesIndexed_Pos, sizeof(Cube::s_VerticesIndexed_Pos), "Cube Vertex Buffer");

		// Create index buffer.
		uint32 numIndices = static_cast<uint32>(Cube::s_Indices.size());
		auto idxBufDesc = AllocIndexBufferDesc(numIndices);
		m_CubeIdxBuf = rm.CreateBuffer(idxBufDesc, &Cube::s_Indices, numIndices * sizeof(uint16), "Cube Index Buffer");

		// Create a structured buffer to hold our instance data and initialize it.
		auto instBufDesc = AllocStructuredBufferDesc(sizeof(InstanceData) * s_NumInstances, sizeof(InstanceData), ResourceUsage::UPLOAD);
		m_CubeInstBuf = rm.CreateBuffer(instBufDesc);

		for (auto& i : m_InstanceData)
		{
			i.modelMatrix = float4x4::identity();
			i.color = { rand() % 255 / 255.0f, rand() % 255 / 255.0f, rand() % 255 / 255.0f };
		}

		// Create a constant buffer
		m_CubeCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();
		m_CubeCB.vtxBufIdx = rm.GetBindlessSRV(m_CubeVtxBuf);
		m_CubeCbvBuf = rm.CreateBuffer(AllocCbvBufferDesc(sizeof(CubeCB)), &m_CubeCB, sizeof(CubeCB));
	}

	~Instancing()
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyTexture(m_ColorRT);
		rm.DestroyTexture(m_DepthRT[DepthBufferMode::STANDARD]);
		rm.DestroyTexture(m_DepthRT[DepthBufferMode::REVERSE_Z]);
		rm.DestroyPipeline(m_FullscreenPso);
		rm.DestroyPipeline(m_CubeInstPso[DepthBufferMode::STANDARD]);
		rm.DestroyPipeline(m_CubeInstPso[DepthBufferMode::REVERSE_Z]);
		rm.DestroyBuffer(m_CubeInstBuf);
		rm.DestroyBuffer(m_CubeCbvBuf);
		rm.DestroyBuffer(m_CubeVtxBuf);
		rm.DestroyBuffer(m_CubeIdxBuf);
	}

	void Update(float dt) override
	{
		// Update the transformation matrices for all cube instances.
		static float v = 0.0f;
		v += 0.01f * dt;

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
		VAST_PROFILE_GPU_BEGIN("Update Buffers", ctx);

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		if (m_bViewChanged)
		{
			m_CubeCB.viewProjMatrix = m_Camera->GetViewProjectionMatrix();
			rm.UpdateBuffer(m_CubeCbvBuf, &m_CubeCB, sizeof(CubeCB));
			m_bViewChanged = false;
		}

		// Upload the instance buffer with the model matrices to render this frame.
		rm.UpdateBuffer(m_CubeInstBuf, &m_InstanceData, sizeof(InstanceData) * s_NumInstances);
		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("Main Render Pass", ctx);

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
		ctx.BeginRenderPass(pso, RenderPassDesc{.rt = { colorTargetDesc }, .ds = depthTargetDesc });
		{
			if (rm.GetIsReady(m_CubeVtxBuf) && rm.GetIsReady(m_CubeIdxBuf) && rm.GetIsReady(m_CubeInstBuf))
			{
				// Bind our instance buffer and CBV.
				ctx.BindConstantBuffer(m_CbvBufProxy, m_CubeCbvBuf);
				ctx.BindSRV(m_InstBufProxy, m_CubeInstBuf);
				// Set the index buffer and draw all instances in a single draw call.
				ctx.BindIndexBuffer(m_CubeIdxBuf, 0, IndexBufFormat::R16_UINT);
				ctx.DrawIndexedInstanced(static_cast<uint32>(Cube::s_Indices.size()), s_NumInstancesToRender, 0, 0, 0);
			}
		}
		ctx.EndRenderPass();

		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("BackBuffer Pass", ctx);

		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD, StoreOp::STORE);
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
		ctx.FlushGPU();

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyTexture(m_ColorRT);
		rm.DestroyTexture(m_DepthRT[DepthBufferMode::STANDARD]);
		rm.DestroyTexture(m_DepthRT[DepthBufferMode::REVERSE_Z]);

		float4 clearColor = float4(0.02f, 0.02f, 0.02f, 1.0f);
		m_ColorRT = rm.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, event.m_WindowSize, clearColor), nullptr, "Color RT");

		auto dsDesc = AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, event.m_WindowSize);
		dsDesc.clear.ds.depth = 1.0f;
		m_DepthRT[DepthBufferMode::STANDARD] = rm.CreateTexture(dsDesc, nullptr, "Depth RT (Standard)");
		dsDesc.clear.ds.depth = 0.0f;
		m_DepthRT[DepthBufferMode::REVERSE_Z] = rm.CreateTexture(dsDesc, nullptr, "Depth RT (Reverse-Z)");

		m_Camera->SetAspectRatio(ctx.GetBackBufferAspectRatio());
		m_bViewChanged = true;
	}

	void OnReloadShadersEvent() override
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.ReloadShaders(m_FullscreenPso);
		rm.ReloadShaders(m_CubeInstPso[DepthBufferMode::STANDARD]);
		rm.ReloadShaders(m_CubeInstPso[DepthBufferMode::REVERSE_Z]);
	}

	void OnGUI() override
	{
		if (ImGui::Begin("Instancing UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::SliderInt("Num Instances to Render", &s_NumInstancesToRender, 0, s_NumInstances);
			if (ImGui::Checkbox("Depth Buffer Use Reverse-Z", &m_bDepthUseReverseZ))
			{
				m_Camera->SetIsReverseZ(m_bDepthUseReverseZ);
				m_bViewChanged = true;
			}
		}
		ImGui::End();
	}
};