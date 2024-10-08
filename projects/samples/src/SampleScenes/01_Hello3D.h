#pragma once

#include "ISample.h"
#include "Rendering/Camera.h"
#include "Rendering/Shapes.h"

using namespace vast;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Hello 3D
 * --------
 * This sample renders a rotating cube to intermediate color and depth targets first, and the color
 * target is then gamma corrected when rendering it to the back buffer on a second render pass. The
 * cube is rendered with an index buffer to avoid storing redundant vertex data. 
 * 
 * All code for this sample is contained within this file plus the shader files '01_Cube.hlsl' and
 * 'Fullscreen.hlsl'.
 *
 * Topics: indexed rendering, render-target, camera matrix, constant buffer, full-screen pass,
 *		   event handling
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
class Hello3D final : public ISample
{
private:
	PipelineHandle m_FullscreenPso;
	TextureHandle m_ColorRT;
	TextureHandle m_DepthRT;

	uint32 m_ColorRTIdx;

	PipelineHandle m_CubePso;
	BufferHandle m_CubeVtxBuf;
	BufferHandle m_CubeIdxBuf;

	BufferHandle m_CubeCbvBuf; ShaderResourceProxy m_CubeCbvBufProxy;

	struct CubeCB
	{
		float4x4 viewProjMatrix;
		uint32 vtxBufIdx;
	} m_CubeCB;

	float4x4 m_CubeModelMatrix;

public:
	Hello3D(GraphicsContext& ctx) : ISample(ctx)
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

		// Create full-screen color and depth intermediate buffers to render our cube to.
		CreateIntermediateRenderTargets(ctx.GetBackBufferSize());

		// Create cube PSO with depth testing enabled.
		m_CubePso = rm.CreatePipeline(PipelineDesc
		{
			.vs = AllocVertexShaderDesc("01_Cube.hlsl", m_SamplesShaderSourcePath.c_str()),
			.ps = AllocPixelShaderDesc("01_Cube.hlsl", m_SamplesShaderSourcePath.c_str()),
			.renderPassLayout = 
			{
				.rtFormats = { rm.GetTextureFormat(m_ColorRT) },
				.dsFormat = { rm.GetTextureFormat(m_DepthRT) },
			},
		});
		// Locate the constant buffer slot in the shader to bind our CBVs
		m_CubeCbvBufProxy = rm.LookupShaderResource(m_CubePso, "ObjectConstantBuffer");

		// Create the cube vertex buffer with bindless access.
		auto vtxBufDesc = AllocVertexBufferDesc(sizeof(Cube::s_VerticesIndexed_Pos), sizeof(Cube::s_VerticesIndexed_Pos[0]));
		m_CubeVtxBuf = rm.CreateBuffer(vtxBufDesc, &Cube::s_VerticesIndexed_Pos, sizeof(Cube::s_VerticesIndexed_Pos), "Cube Vertex Buffer");

		// Create the index buffer.
		uint32 numIndices = static_cast<uint32>(Cube::s_Indices.size());
		auto idxBufDesc = AllocIndexBufferDesc(numIndices);
		m_CubeIdxBuf = rm.CreateBuffer(idxBufDesc, &Cube::s_Indices, numIndices * sizeof(uint16), "Cube Index Buffer");

		// Create a constant buffer and fill it with the necessary constant data for rendering the cube.
		m_CubeCB.viewProjMatrix = ComputeViewProjectionMatrix();
		m_CubeCB.vtxBufIdx = rm.GetBindlessSRV(m_CubeVtxBuf);

		m_CubeCbvBuf = rm.CreateBuffer(AllocCbvBufferDesc(sizeof(CubeCB)), &m_CubeCB, sizeof(CubeCB), "Cube Constant Buffer");

		m_CubeModelMatrix = float4x4::identity();
	}

	~Hello3D()
	{
		// Clean up GPU resources created for this sample.
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyPipeline(m_FullscreenPso);
		rm.DestroyPipeline(m_CubePso);
		rm.DestroyBuffer(m_CubeVtxBuf);
		rm.DestroyBuffer(m_CubeIdxBuf);
		rm.DestroyBuffer(m_CubeCbvBuf);

		DestroyIntermediateRenderTargets();
	}

	void Update(float dt) override
	{
		// Rotate the cube transform slowly.
		static float rotation = 0.0f;
		rotation += 0.001f * dt;
		m_CubeModelMatrix = float4x4::rotation_y(rotation);
	}

	void Render() override
	{		
		VAST_PROFILE_GPU_BEGIN("Cube Render Pass", ctx);

		// Transition and clear our intermediate color and depth targets and set the pipeline state
		// to render the cube.
		const RenderPassDesc rpDesc =
		{
			.rt = { RenderTargetDesc{.h = m_ColorRT, .loadOp = LoadOp::CLEAR } },
			.ds = {.h = m_DepthRT, .loadOp = LoadOp::CLEAR, .storeOp = StoreOp::DISCARD }
		};

		ctx.BeginRenderPass(m_CubePso, rpDesc);
		{
			// Bind our constant buffer containing the bindless index to the vertex buffer and camera matrix
			// information. The model matrix is passed via push constants so that we only have to update the
			// constant buffer data once.
			ctx.SetPushConstants(&m_CubeModelMatrix, sizeof(float4x4));
			ctx.BindConstantBuffer(m_CubeCbvBufProxy, m_CubeCbvBuf);
			ctx.BindIndexBuffer(m_CubeIdxBuf);
			ctx.DrawIndexed(36);
		}
		ctx.EndRenderPass();

		VAST_PROFILE_GPU_END();
		VAST_PROFILE_GPU_BEGIN("BackBuffer Pass", ctx);

		// Render our color target to the back buffer and gamma correct it. Because we render to all pixels
		// every frame, we don't need to clear the back buffer.
		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD);
		{
			ctx.SetPushConstants(&m_ColorRTIdx, sizeof(uint32));
			ctx.DrawFullscreenTriangle();
		}
		ctx.EndRenderPass();

		VAST_PROFILE_GPU_END();
	}

	void OnWindowResizeEvent(const WindowResizeEvent& event) override
	{
		// If the window is resized we need to update the size of the render targets, as well as our camera aspect ratio.
		ctx.FlushGPU();

		DestroyIntermediateRenderTargets();
		CreateIntermediateRenderTargets(event.m_WindowSize);

		m_CubeCB.viewProjMatrix = ComputeViewProjectionMatrix();

		GPUResourceManager& rm = ctx.GetGPUResourceManager();
		rm.UpdateBuffer(m_CubeCbvBuf, &m_CubeCB, sizeof(m_CubeCB));
	}

	void OnReloadShadersEvent() override
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.ReloadShaders(m_FullscreenPso);
		rm.ReloadShaders(m_CubePso);
	}

	void CreateIntermediateRenderTargets(uint2 dimensions)
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		const float4 clearColor = float4(0.6f, 0.2f, 0.3f, 1.0f);
		m_ColorRT = rm.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, dimensions, clearColor), nullptr, "Color RT");
		m_DepthRT = rm.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, dimensions), nullptr, "Depth RT");
		m_ColorRTIdx = rm.GetBindlessSRV(m_ColorRT);
	}

	void DestroyIntermediateRenderTargets()
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyTexture(m_ColorRT);
		rm.DestroyTexture(m_DepthRT);
	}

	float4x4 ComputeViewProjectionMatrix()
	{
		float4x4 viewMatrix = Camera::ComputeViewMatrix(float3(-3.0f, 3.0f, -8.0f), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 projMatrix = Camera::ComputeProjectionMatrix(DEG_TO_RAD(30.0f), ctx.GetBackBufferAspectRatio(), 0.001f, 10.0f);
		return Camera::ComputeViewProjectionMatrix(viewMatrix, projMatrix);
	}

};