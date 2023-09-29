#include "vastpch.h"
#include "Rendering/ImguiRenderer.h"

#include "Rendering/Imgui.h"

#define USE_IMGUI_STOCK_IMPL_WIN32	1

#if USE_IMGUI_STOCK_IMPL_WIN32
#include "imgui/backends/imgui_impl_win32.h"
#endif

namespace vast::gfx
{

	ImguiRenderer::ImguiRenderer(gfx::GraphicsContext& ctx_, HWND windowHandle /*= ::GetActiveWindow()*/)
		: ctx(ctx_)
	{
		VAST_PROFILE_TRACE_SCOPE("ui", "Create Imgui Renderer");
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Init(windowHandle);
#endif

		BlendState bs = BlendState::Preset::kAdditive;
		bs.dstBlendAlpha = Blend::INV_SRC_ALPHA;

		PipelineDesc pipelineDesc =
		{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "imgui.hlsl", .entryPoint = "VS_Imgui"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "imgui.hlsl", .entryPoint = "PS_Imgui"},
			.blendStates = { bs },
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
		};
		m_Pipeline = ctx.CreatePipeline(pipelineDesc);
		m_CbvProxy = ctx.LookupShaderResource(m_Pipeline, "ImguiCB");

		ImGuiIO& io = ImGui::GetIO();
		unsigned char* texData;
		int32 texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&texData, &texWidth, &texHeight);

		TextureDesc texDesc =
		{
			.format = TexFormat::RGBA8_UNORM_SRGB,
			.width = static_cast<uint32>(texWidth),
			.height = static_cast<uint32>(texHeight),
			.viewFlags = TexViewFlags::SRV,
		};
		m_FontTex = ctx.CreateTexture(texDesc, texData);

		io.Fonts->SetTexID(&m_FontTex);
	}

	ImguiRenderer::~ImguiRenderer()
	{
		VAST_PROFILE_TRACE_SCOPE("ui", "Destroy Imgui Renderer");
		ctx.DestroyTexture(m_FontTex);
		ctx.DestroyPipeline(m_Pipeline);

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Shutdown();
#endif
	}

	void ImguiRenderer::BeginCommandRecording()
	{
		VAST_PROFILE_TRACE_SCOPE("ui", "Begin Command Recording (UI)");
#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_NewFrame();
#endif
		ImGui::NewFrame();
	}

	void ImguiRenderer::EndCommandRecording()
	{
		VAST_PROFILE_TRACE_SCOPE("ui", "End Command Recording (UI)");
		ImGui::Render();
	}

	static const float4x4 ComputeProjectionMatrix(ImDrawData* data)
	{
		float L = data->DisplayPos.x;
		float R = data->DisplayPos.x + data->DisplaySize.x;
		float T = data->DisplayPos.y;
		float B = data->DisplayPos.y + data->DisplaySize.y;
		return float4x4(
			float4(2.0f / (R - L),		0.0f,				0.0f,				0.0f),
			float4(0.0f,				2.0f / (T - B),		0.0f,				0.0f),
			float4(0.0f,				0.0f,				0.5f,				0.0f),
			float4((R + L) / (L - R),	(T + B) / (B - T),	0.5f,				1.0f)
		);
	}

	void ImguiRenderer::Render()
	{
		VAST_PROFILE_TRACE_SCOPE("ui", "Render (UI)");
		VAST_ASSERTF(ctx.IsInFrame() && !ctx.IsInRenderPass(), "This function must be invoked within Graphics Context frame bounds and out of a Render Pass.");

		ImDrawData* drawData = ImGui::GetDrawData();

		if (!drawData || drawData->TotalVtxCount == 0)
			return;

		// Avoid rendering when minimized
		if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
			return;

		// Get memory for vertex and index buffers
		VAST_PROFILE_TRACE_BEGIN("ui", "Merge Buffers");
		const uint32 vtxSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
		const uint32 idxSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
		BufferView vtxView = ctx.AllocTempBufferView(vtxSize, 4); // TODO: Do we need 4 alignment?
		BufferView idxView = ctx.AllocTempBufferView(idxSize, 4);

		// Copy vertex and index buffer data to a single buffer
		ImDrawVert* vtxCpuMem = reinterpret_cast<ImDrawVert*>(vtxView.data);
		ImDrawIdx* idxCpuMem = reinterpret_cast<ImDrawIdx*>(idxView.data);
		for (int32 i = 0; i < drawData->CmdListsCount; ++i)
		{
			const ImDrawList* drawList = drawData->CmdLists[i];
			memcpy(vtxCpuMem, &drawList->VtxBuffer[0], drawList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxCpuMem, &drawList->IdxBuffer[0], drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxCpuMem += drawList->VtxBuffer.Size;
			idxCpuMem += drawList->IdxBuffer.Size;
		}
		VAST_PROFILE_TRACE_END("ui", "Merge Buffers");

		ctx.BeginRenderPassToBackBuffer(m_Pipeline, LoadOp::LOAD, StoreOp::STORE);
		{
			ctx.BindVertexBuffer(vtxView.buffer, vtxView.offset, sizeof(ImDrawVert));
			ctx.BindIndexBuffer(idxView.buffer, idxView.offset, IndexBufFormat::R16_UINT);

			// Bind temporary CBV
			const float4x4 mvp = ComputeProjectionMatrix(drawData);
			BufferView cbv = ctx.AllocTempBufferView(sizeof(float4x4), CONSTANT_BUFFER_ALIGNMENT);
			memcpy(cbv.data, &mvp, sizeof(float4x4));
			ctx.BindConstantBuffer(m_CbvProxy, cbv.buffer, cbv.offset);

			ctx.SetBlendFactor(float4(0));
			uint32 vtxOffset = 0, idxOffset = 0;
			ImVec2 clipOff = drawData->DisplayPos;
			VAST_PROFILE_TRACE_BEGIN("ui", "Draw Loop");
			for (int32 i = 0; i < drawData->CmdListsCount; ++i)
			{
				const ImDrawList* drawList = drawData->CmdLists[i];

				for (int32 j = 0; j < drawList->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* drawCmd = &drawList->CmdBuffer[j];
					if (drawCmd->UserCallback != NULL)
					{
						drawCmd->UserCallback(drawList, drawCmd);
					}
					else
					{
						ImVec2 clipMin(drawCmd->ClipRect.x - clipOff.x, drawCmd->ClipRect.y - clipOff.y);
						ImVec2 clipMax(drawCmd->ClipRect.z - clipOff.x, drawCmd->ClipRect.w - clipOff.y);
						if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
						{
							continue;
						}

						TextureHandle h = *drawCmd->GetTexID();

						if (!ctx.GetIsReady(h))
							continue;

						// TODO: Do we need explicit transitions here? If so we could bundle them in one go.
						// TODO: Support choosing texture filtering.
						// TODO: Support cubemap images.
						uint32 srvIndex = ctx.GetBindlessSRV(h);
						ctx.SetPushConstants(&srvIndex, sizeof(uint32));
						ctx.SetScissorRect(int4(clipMin.x, clipMin.y, clipMax.x, clipMax.y));
						ctx.DrawIndexedInstanced(drawCmd->ElemCount, 1, drawCmd->IdxOffset + idxOffset, drawCmd->VtxOffset + vtxOffset, 0);
					}
				}
				idxOffset += drawList->IdxBuffer.Size;
				vtxOffset += drawList->VtxBuffer.Size;
			}
			VAST_PROFILE_TRACE_END("ui", "Draw Loop");
		}
		ctx.EndRenderPass();
	}

}
