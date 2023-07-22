#pragma once

#include "ISample.h"
#include "Rendering/Camera.h"

using namespace vast::gfx;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Hello 3D
 * --------
 * This sample renders a rotating textured cube onto the screen. The cube vertex buffer is static
 * this time around and resident in VRAM instead of in the CPU. We render the cube to intermediate
 * color and depth targets and then gamma correct the color target when rendering it to the back
 * buffer on a second render pass. The cube has a color texture loaded from the assets folder. Both
 * the vertex buffer and the texture are accessed bindlessly in the shader, with the necessary heap
 * indices stored in a constant buffer alongside the model view and projection matrices used for 
 * rendering. This sample also show how to handle events such as a window resize event.
 *
 * All code for this sample is contained within this file plus the shader files 'cube.hlsl' and
 * 'fullscreen.hlsl' containing code for rendering the cube and gamma correcting the color result
 * to the back buffer respectively.
 *
 * Topics: full-screen triangle, render targets and depth target, texture loading, constant buffer,
 *         camera matrices, clear color
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
class Hello3D final : public ISample
{
private:
	PipelineHandle m_FullscreenPso;
	TextureHandle m_ColorRT;
	TextureHandle m_DepthRT;

	PipelineHandle m_CubePso;
	TextureHandle m_CubeColorTex;
	BufferHandle m_CubeVtxBuf;
	BufferHandle m_CubeCbvBuf; ShaderResourceProxy m_CubeCbvBufProxy;

	struct CubeCB
	{
		float4x4 viewProjMatrix;
		float4x4 modelMatrix;
		uint32 vtxBufIdx;
		uint32 colTexIdx;
	} m_CubeCB;

	struct Vtx3fPos2fUv
	{
		s_float3 pos;
		s_float2 uv;
	};

public:
	Hello3D(GraphicsContext& ctx_) : ISample(ctx_)
	{	
		// Create full-screen pass PSO
		m_FullscreenPso = ctx.CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "fullscreen.hlsl", .entryPoint = "VS_Main"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "fullscreen.hlsl", .entryPoint = "PS_Main"},
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
		});

		// Create full-screen color and depth intermediate buffers to render our cube to.
		uint2 backBufferSize = ctx.GetBackBufferSize();
		float4 clearColor = float4(0.6f, 0.2f, 0.3f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, backBufferSize, clearColor));
		m_DepthRT = ctx.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, backBufferSize));

		// Create cube PSO with depth testing enabled.
		m_CubePso = ctx.CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "cube.hlsl", .entryPoint = "VS_Cube"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "cube.hlsl", .entryPoint = "PS_Cube"},
			.renderPassLayout = 
			{
				.rtFormats = { ctx.GetTextureFormat(m_ColorRT) },
				.dsFormat = { ctx.GetTextureFormat(m_DepthRT) },
			},
		});
		// Locate the constant buffer slot in the shader to bind our CBVs
		m_CubeCbvBufProxy = ctx.LookupShaderResource(m_CubePso, "ObjectConstantBuffer");

		Array<Vtx3fPos2fUv, 36> cubeVertexData =
		{ {
			// Top
			{{ 1.0f,-1.0f, 1.0f }, { 1.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f }},
			// Bottom
			{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }},
			// Left
			{{-1.0f,-1.0f,-1.0f }, { 1.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }},
			// Front
			{{-1.0f,-1.0f, 1.0f }, { 1.0f, 1.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 1.0f }},
			{{-1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }},
			// Right
			{{ 1.0f,-1.0f, 1.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f,-1.0f,-1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f }},
			// Back
			{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 1.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 1.0f }},
			{{-1.0f,-1.0f,-1.0f }, { 0.0f, 1.0f }},
			{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f }},
			{{-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f }},
		} };

		// Create the cube vertex buffer with bindless access.
		auto vtxBufDesc = AllocVertexBufferDesc(sizeof(cubeVertexData), sizeof(cubeVertexData[0]));
		m_CubeVtxBuf = ctx.CreateBuffer(vtxBufDesc, &cubeVertexData, sizeof(cubeVertexData));

		// Load a texture from file to be used on the cube
		m_CubeColorTex = ctx.CreateTexture("image.tga");

		// Create a constant buffer and fill it with the necessary data for rendering the cube.
		m_CubeCB.viewProjMatrix = ComputeViewProjectionMatrix();
		m_CubeCB.modelMatrix = float4x4::identity();
		m_CubeCB.vtxBufIdx = ctx.GetBindlessIndex(m_CubeVtxBuf);
		m_CubeCB.colTexIdx = ctx.GetBindlessIndex(m_CubeColorTex);

		m_CubeCbvBuf = ctx.CreateBuffer(AllocCbvBufferDesc(sizeof(CubeCB)), &m_CubeCB, sizeof(CubeCB));
		
		// TODO: Ideally we'd subscribe the base class and that would invoke the derived class... likely not possible.
		VAST_SUBSCRIBE_TO_EVENT("hello3d", WindowResizeEvent, VAST_EVENT_HANDLER_CB(Hello3D::OnWindowResizeEvent, WindowResizeEvent));
	}

	~Hello3D()
	{
		// Clean up GPU resources created for this sample.
		ctx.DestroyPipeline(m_FullscreenPso);
		ctx.DestroyPipeline(m_CubePso);
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT);
		ctx.DestroyTexture(m_CubeColorTex);
		ctx.DestroyBuffer(m_CubeVtxBuf);
		ctx.DestroyBuffer(m_CubeCbvBuf);

		VAST_UNSUBSCRIBE_FROM_EVENT("hello3d", WindowResizeEvent);
	}

	void Update() override
	{
		// Rotate the cube model matrix slowly.
		static float rotation = 0.0f;
		rotation += 0.001f;
		m_CubeCB.modelMatrix = float4x4::rotation_y(rotation);
	}

	void Render() override
	{
		// Upload the rotated model matrix and updated camera settings to render this frame.
		ctx.UpdateBuffer(m_CubeCbvBuf, &m_CubeCB, sizeof(CubeCB));

		const RenderTargetDesc colorTargetDesc = {.h = m_ColorRT, .loadOp = LoadOp::CLEAR };
		const RenderTargetDesc depthTargetDesc = {.h = m_DepthRT, .loadOp = LoadOp::CLEAR, .storeOp = StoreOp::DISCARD };
		// Transition and clear our intermediate color and depth targets and set the pipeline state
		// to render the cube.
		ctx.BeginRenderPass(m_CubePso, RenderPassTargets{.rt = { colorTargetDesc }, .ds = depthTargetDesc });
		{
			if (ctx.GetIsReady(m_CubeVtxBuf) && ctx.GetIsReady(m_CubeColorTex))
			{
				// Bind our constant buffer containing the bindless indices to the vertex buffer
				// and the color texture.
				ctx.SetConstantBufferView(m_CubeCbvBuf, m_CubeCbvBufProxy);
				ctx.Draw(36);
			}
		}
		ctx.EndRenderPass();

		// Render our color target to the back buffer and gamma correct it.
		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD, StoreOp::STORE);
		{
			uint32 srvIndex = ctx.GetBindlessIndex(m_ColorRT);
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
		m_CubeCB.viewProjMatrix = ComputeViewProjectionMatrix();
	}

	float4x4 ComputeViewProjectionMatrix()
	{
		float4x4 viewMatrix = Camera::ComputeViewMatrix(float3(-3.0f, 3.0f, -8.0f), float3(0, 0, 0), float3(0, 1, 0));
		float4x4 projMatrix = Camera::ComputeProjectionMatrix(DEG_TO_RAD(30.0f), ctx.GetBackBufferAspectRatio(), 0.001f, 10.0f);
		return Camera::ComputeViewProjectionMatrix(viewMatrix, projMatrix);
	}
};