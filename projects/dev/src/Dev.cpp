#include "Dev.h"

#include "shaders_shared.h"

#include "imgui/imgui.h"

// ---------------------------------------- TODO LIST ------------------------------------------ //
// Features coming up:
//
//	> Shader Precompilation: precompile shaders to avoid compiling at every start-up.
//	> Shader Visual Studio integration 2: shader compilation from solution.
//		- req: Shader Precompilation
//
//	> GFX Rasterizer State.
//	> GFX Stencil State.
//	> GFX Render Pass / Render Pass Layout.
//	> GFX Compute Shaders.
//	> GFX Display List: command recording for later execution.
//	> GFX Multithreading.
//		- req: GFX Display List
//
// --------------------------------------------------------------------------------------------- //

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

static Array<TriangleVtx, 3> s_TriangleVertexData =
{ {
	{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
	{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
	{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
} };
static bool s_UpdateTriangle = false;
static bool s_ReloadTriangle = false;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
	auto windowSize = m_Window->GetWindowSize();

	gfx::GraphicsParams params;
	params.swapChainSize = windowSize;
	params.swapChainFormat = gfx::Format::RGBA8_UNORM;
	params.backBufferFormat = gfx::Format::RGBA8_UNORM;

	m_GraphicsContext = gfx::GraphicsContext::Create(params);
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	m_ClearColorRT.flags = gfx::ClearFlags::CLEAR_COLOR;
	m_ClearColorRT.color = float4(0.6f, 0.2f, 0.9f, 1.0f);

	m_ColorRT = m_GraphicsContext->CreateTexture(gfx::TextureDesc::Builder()
		.Type(gfx::TextureType::TEXTURE_2D)
		.Format(gfx::Format::RGBA8_UNORM)
		.Width(windowSize.x)
		.Height(windowSize.y)
		.ViewFlags(gfx::TextureViewFlags::RTV | gfx::TextureViewFlags::SRV));
	m_ColorTextureIdx = ctx.GetBindlessIndex(m_ColorRT);

	m_FullscreenPso = ctx.CreatePipeline(gfx::PipelineDesc::Builder()
		.VS("fullscreen.hlsl", "VS_Main")
		.PS("fullscreen.hlsl", "PS_Main")
		.DepthStencil(gfx::DepthStencilState::Preset::kDisabled)
		.SetRenderTarget(ctx.GetBackBufferFormat()));

	CreateTriangleResources();

	m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(*m_GraphicsContext);
}

void Dev::CreateTriangleResources()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	m_TrianglePso = ctx.CreatePipeline(gfx::PipelineDesc::Builder()
		.VS("triangle.hlsl", "VS_Main")
		.PS("triangle.hlsl", "PS_Main")
		.DepthStencil(gfx::DepthStencilState::Preset::kDisabled)
		.SetRenderTarget(ctx.GetTextureFormat(m_ColorRT)));

	auto vtxBufDesc = gfx::BufferDesc::Builder()
		.Size(sizeof(s_TriangleVertexData)).Stride(sizeof(s_TriangleVertexData[0]))
		.ViewFlags(gfx::BufferViewFlags::SRV)
		.CpuAccess(gfx::BufferCpuAccess::WRITE)
		.Usage(gfx::ResourceUsage::DYNAMIC)
		.IsRawAccess(true);
	m_TriangleVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_TriangleVertexData, sizeof(s_TriangleVertexData));
	m_TriangleVtxBufIdx = ctx.GetBindlessIndex(m_TriangleVtxBuf);
}

Dev::~Dev()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.DestroyPipeline(m_FullscreenPso);
	ctx.DestroyPipeline(m_TrianglePso);
	ctx.DestroyTexture(m_ColorRT);
	ctx.DestroyBuffer(m_TriangleVtxBuf);
}

void Dev::OnUpdate()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.BeginFrame();
	m_ImguiRenderer->BeginFrame();

	if (s_ReloadTriangle)
	{
		s_ReloadTriangle = false;
		ctx.UpdatePipeline(m_TrianglePso);
	}

	if (s_UpdateTriangle)
	{
		s_UpdateTriangle = false;
		ctx.UpdateBuffer(m_TriangleVtxBuf, &s_TriangleVertexData, sizeof(s_TriangleVertexData));
	}

	ctx.SetRenderTarget(m_ColorRT);
	ctx.BeginRenderPass(m_TrianglePso, m_ClearColorRT);
	{
		ctx.SetPushConstants(&m_TriangleVtxBufIdx, sizeof(uint32));
		ctx.Draw(3);
	}
	ctx.EndRenderPass();

	ctx.BeginRenderPass(m_FullscreenPso);
	{
		ctx.SetPushConstants(&m_ColorTextureIdx, sizeof(uint32));
		ctx.DrawFullscreenTriangle();
	}
	ctx.EndRenderPass();

	OnGUI();

	m_ImguiRenderer->EndFrame();
	ctx.EndFrame();
}

void Dev::OnGUI()
{
	ImGui::ShowDemoWindow();

	if (ImGui::Begin("vast UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::PushItemWidth(300);
		ImGui::Text("Triangle Vertex Positions");
		if (ImGui::SliderFloat2("##1", (float*)&s_TriangleVertexData[0].pos, -1.0f, 1.0f)) s_UpdateTriangle = true;
		if (ImGui::SliderFloat2("##2", (float*)&s_TriangleVertexData[1].pos, -1.0f, 1.0f)) s_UpdateTriangle = true;
		if (ImGui::SliderFloat2("##3", (float*)&s_TriangleVertexData[2].pos, -1.0f, 1.0f)) s_UpdateTriangle = true;
		ImGui::Text("Triangle Vertex Colors");
		if (ImGui::ColorEdit3("##1", (float*)&s_TriangleVertexData[0].col)) s_UpdateTriangle = true;
		if (ImGui::ColorEdit3("##2", (float*)&s_TriangleVertexData[1].col)) s_UpdateTriangle = true;
		if (ImGui::ColorEdit3("##3", (float*)&s_TriangleVertexData[2].col)) s_UpdateTriangle = true;
		ImGui::PopItemWidth();
		ImGui::Separator();
		ImGui::PushItemWidth(300);
		ImGui::Text("Clear color");
		ImGui::ColorEdit4("##bgcol", (float*)&m_ClearColorRT.color);
		ImGui::PopItemWidth();
		if (ImGui::Button("Reload Triangle Shader"))
		{
			s_ReloadTriangle = true;
		}
	}
	ImGui::End();
}
