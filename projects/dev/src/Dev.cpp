#include "Dev.h"

#include "shaders_shared.h"

#include "imgui/imgui.h"

// ---------------------------------------- TODO LIST ------------------------------------------ //
// Features coming up:
//
//	> Shader Hot Reload: allow shaders to be recompiled and reloaded live.
//	> Shader Precompilation: precompile shaders to avoid compiling at every start-up.
//	> Shader Visual Studio integration 1: syntax coloring for hlsl files.
//	> Shader Visual Studio integration 2: shader compilation from solution.
//		- req: Shader Precompilation
//
//	> GFX Rasterizer State.
//	> GFX Stencil State.
//	> GFX Samplers.
//	> GFX Render Pass / Render Pass Layout.
//	> GFX Compute Shaders.
//	> GFX Display List: command recording for later execution.
//	> GFX Multithreading.
//		- req: GFX Display List
//
// --------------------------------------------------------------------------------------------- //

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
	auto windowSize = m_Window->GetWindowSize();

	gfx::GraphicsParams params;
	params.swapChainSize = windowSize;
	params.swapChainFormat = gfx::Format::RGBA8_UNORM;
	params.backBufferFormat = gfx::Format::RGBA8_UNORM;

 	m_GraphicsContext = gfx::GraphicsContext::Create(params);

	{
		m_ColorRT = m_GraphicsContext->CreateTexture(gfx::TextureDesc::Builder()
			.Type(gfx::TextureType::TEXTURE_2D)
			.Format(gfx::Format::RGBA8_UNORM)
			.Width(windowSize.x)
			.Height(windowSize.y)
			.ViewFlags(gfx::TextureViewFlags::RTV | gfx::TextureViewFlags::SRV));
	}

	CreateTriangleResources();
	CreateFullscreenPassResources();

	m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(*m_GraphicsContext);
}

void Dev::CreateTriangleResources()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	{
		auto pipelineDesc = gfx::PipelineDesc::Builder()
			.VS("triangle.hlsl", "VS_Main")
			.PS("triangle.hlsl", "PS_Main")
			.DepthStencil(gfx::DepthStencilState::Preset::kDisabled)
			.SetRenderTarget(gfx::Format::RGBA8_UNORM); // TODO: m_ColorRT->GetFormat()
		m_TrianglePso = ctx.CreatePipeline(pipelineDesc);

		m_ClearColorRT.flags = gfx::ClearFlags::CLEAR_COLOR;
		m_ClearColorRT.color = float4(0.6f, 0.2f, 0.9f, 1.0f);
	}
	{
		Array<TriangleVtx, 3> vertexData =
		{ {
			{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
			{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
			{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
		} };

		auto vtxBufDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(vertexData)).Stride(sizeof(vertexData[0]))
			.ViewFlags(gfx::BufferViewFlags::SRV)
			.CpuAccess(gfx::BufferCpuAccess::WRITE)
			.Usage(gfx::ResourceUsage::DYNAMIC)
			.IsRawAccess(true);

		m_TriangleVtxBuf = ctx.CreateBuffer(vtxBufDesc, &vertexData, sizeof(vertexData));

		TriangleCBV cbvData = { ctx.GetBindlessIndex(m_TriangleVtxBuf) };

		auto cbvDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(TriangleCBV))
			.ViewFlags(gfx::BufferViewFlags::CBV);

		m_TriangleCbv = ctx.CreateBuffer(cbvDesc, &cbvData, sizeof(cbvData));

		m_TriangleCbvProxy = ctx.LookupShaderResource(m_TrianglePso, "ObjectConstantBuffer");
	}
}

void Dev::CreateFullscreenPassResources()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	{
		auto pipelineDesc = gfx::PipelineDesc::Builder()
			.VS("fullscreen.hlsl", "VS_Main")
			.PS("fullscreen.hlsl", "PS_Main")
			.DepthStencil(gfx::DepthStencilState::Preset::kDisabled)
			.SetRenderTarget(ctx.GetBackBufferFormat());
		m_FullscreenPso = ctx.CreatePipeline(pipelineDesc);

		m_ClearColorRT.flags = gfx::ClearFlags::CLEAR_COLOR;
		m_ClearColorRT.color = float4(0.6f, 0.2f, 0.9f, 1.0f);
	}
	{
		FullscreenCBV cbvData = { ctx.GetBindlessIndex(m_ColorRT) };

		auto cbvDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(FullscreenCBV))
			.ViewFlags(gfx::BufferViewFlags::CBV);

		m_FullscreenCbv = ctx.CreateBuffer(cbvDesc, &cbvData, sizeof(cbvData));

		m_FullscreenCbvProxy = ctx.LookupShaderResource(m_TrianglePso, "ObjectConstantBuffer");
	}
}

Dev::~Dev()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.DestroyTexture(m_ColorRT);

	ctx.DestroyPipeline(m_TrianglePso);
	ctx.DestroyBuffer(m_TriangleVtxBuf);
	ctx.DestroyBuffer(m_TriangleCbv);

	ctx.DestroyPipeline(m_FullscreenPso);
	ctx.DestroyBuffer(m_FullscreenCbv);
}

void Dev::OnUpdate()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	static float tempTimeCounter = 0;
	static int tempTimeThreshold = 1;
	tempTimeCounter += 0.01f;

	ctx.BeginFrame();
	m_ImguiRenderer->BeginFrame();

	if (tempTimeCounter >= float(tempTimeThreshold))
	{
		tempTimeThreshold++;

		Array<TriangleVtx, 3> vertexData =
		{ {
			{{  sin(tempTimeCounter), -0.5f }, { 0.5f, 1.0f, 0.3f }},
			{{  0.0f, sin(tempTimeCounter * 12 + 4) }, { 0.1f, 0.6f, 0.7f }},
			{{  cos(tempTimeCounter), -0.5f }, { 0.4f, 0.2f, 0.0f }},
		} };

		ctx.UpdateBuffer(m_TriangleVtxBuf, &vertexData, sizeof(vertexData));
	}

	ctx.SetRenderTarget(m_ColorRT);
	ctx.BeginRenderPass(m_TrianglePso, m_ClearColorRT);
	{
		ctx.SetShaderResource(m_TriangleCbv, m_TriangleCbvProxy);
		ctx.Draw(3);
	}
	ctx.EndRenderPass();

	ctx.BeginRenderPass(m_FullscreenPso/*, m_ClearColorRT*/);
	{
		ctx.SetShaderResource(m_FullscreenCbv, m_FullscreenCbvProxy);
		ctx.DrawFullscreenTriangle();
	}
	ctx.EndRenderPass();

	ImGui::ShowDemoWindow();

	m_ImguiRenderer->EndFrame();
	ctx.EndFrame();
}
