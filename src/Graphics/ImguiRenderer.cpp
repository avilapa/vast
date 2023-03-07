#include "vastpch.h"
#include "Graphics/ImguiRenderer.h"

#include "imgui/imgui.h"

#define USE_IMGUI_STOCK_IMPL_WIN32	1

#if USE_IMGUI_STOCK_IMPL_WIN32
#include "imgui/backends/imgui_impl_win32.h"
#endif

#define IMGUI_VTX_BUF_INIT_SIZE	5000
#define IMGUI_IDX_BUF_INIT_SIZE	10000
#define IMGUI_VTX_BUF_STEP_SIZE	5000
#define IMGUI_IDX_BUF_STEP_SIZE	10000

namespace vast::gfx
{

	ImguiRenderer::ImguiRenderer(gfx::GraphicsContext& context, HWND windowHandle /*= ::GetActiveWindow()*/)
		: ctx(context)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Init(windowHandle);
#endif

		// Create font texture SRV and upload texture data
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* texData;
		int32 texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&texData, &texWidth, &texHeight);

		auto texDesc = TextureDesc::Builder()
			.Type(TextureType::TEXTURE_2D)
			.Format(Format::RGBA8_UNORM_SRGB)
			.Width(texWidth)
			.Height(texHeight)
			.DepthOrArraySize(1)
			.MipCount(1)
			.ViewFlags(TextureViewFlags::SRV);

		m_FontTex = ctx.CreateTexture(texDesc, texData);

		// Create vertex and index buffers with an initial size
		auto vtxBufDesc = BufferDesc::Builder().Size(static_cast<uint32>(IMGUI_VTX_BUF_INIT_SIZE * sizeof(ImDrawVert))).Stride(sizeof(ImDrawVert));
		auto idxBufDesc = BufferDesc::Builder().Size(static_cast<uint32>(IMGUI_IDX_BUF_INIT_SIZE * sizeof(ImDrawIdx))).Stride(sizeof(ImDrawIdx));

		for (uint32 i = 0; i < IMGUI_NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_VtxSize[i] = IMGUI_VTX_BUF_INIT_SIZE;
			m_IdxSize[i] = IMGUI_IDX_BUF_INIT_SIZE;
			m_VtxBuf[i] = ctx.CreateBuffer(vtxBufDesc);
			m_IdxBuf[i] = ctx.CreateBuffer(idxBufDesc);
		}

		auto pipelineDesc = PipelineDesc::Builder()
			.VS("imgui.hlsl", "VS_Main")
			.PS("imgui.hlsl", "PS_Main")
			// TODO: Pipeline State
			.SetRenderTarget(gfx::Format::RGBA8_UNORM_SRGB); // TODO: This should internally query the backbuffer format.

		m_Pipeline = ctx.CreatePipeline(pipelineDesc);
	}

	ImguiRenderer::~ImguiRenderer()
	{
		for (uint32 i = 0; i < IMGUI_NUM_FRAMES_IN_FLIGHT; ++i)
		{
			ctx.DestroyBuffer(m_VtxBuf[i]);
			ctx.DestroyBuffer(m_IdxBuf[i]);
		}
		ctx.DestroyTexture(m_FontTex);
		ctx.DestroyPipeline(m_Pipeline);

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Shutdown();
#endif
	}

	void ImguiRenderer::BeginFrame()
	{
#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_NewFrame();
#endif
		ImGui::NewFrame();
	}

	void ImguiRenderer::ExecuteRenderPass()
	{

	}
}
