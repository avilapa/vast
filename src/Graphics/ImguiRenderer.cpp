#include "vastpch.h"
#include "Graphics/ImguiRenderer.h"

#include "imgui/imgui.h"

#define USE_IMGUI_STOCK_IMPL_WIN32	1

#if USE_IMGUI_STOCK_IMPL_WIN32
#include "imgui/backends/imgui_impl_win32.h"
#endif

namespace vast::gfx
{

	ImguiRenderer::ImguiRenderer(gfx::GraphicsContext& context, HWND windowHandle /*= ::GetActiveWindow()*/)
		: ctx(context)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Init(windowHandle);
#endif

		{
			BlendState bs = BlendState::Preset::kAdditive;
			bs.dstBlendAlpha = Blend::INV_SRC_ALPHA;

			auto pipelineDesc = PipelineDesc::Builder()
				.VS("imgui.hlsl", "VS_Main")
				.PS("imgui.hlsl", "PS_Main")
				.DepthStencil(DepthStencilState::Preset::kDisabled)
				.SetRenderTarget(ctx.GetBackBufferFormat(), bs);

			m_Pipeline = ctx.CreatePipeline(pipelineDesc);
		}

		{
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

			m_FontTexProxy = ctx.LookupShaderResource(m_Pipeline, "FontTexture");
		}
	}

	ImguiRenderer::~ImguiRenderer()
	{
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

	static const float4x4 ComputeProjectionMatrix(ImDrawData* data)
	{
		float L = data->DisplayPos.x;
		float R = data->DisplayPos.x + data->DisplaySize.x;
		float T = data->DisplayPos.y;
		float B = data->DisplayPos.y + data->DisplaySize.y;
		return float4x4(
			float4(2.0f / (R - L), 0.0f, 0.0f, 0.0f),
			float4(0.0f, 2.0f / (T - B), 0.0f, 0.0f),
			float4(0.0f, 0.0f, 0.5f, 0.0f),
			float4((R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f)
		);
	}

	void ImguiRenderer::EndFrame()
	{
		ImGui::Render();

		ImDrawData* drawData = ImGui::GetDrawData();

		if (!drawData || drawData->TotalVtxCount == 0)
			return;

		// Avoid rendering when minimized
		if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
			return;

		if (!ctx.GetIsReady(m_FontTex))
			return;

		// Get memory for vertex and index buffers
		const uint32 vtxSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
		const uint32 idxSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
		BufferView vtxView = ctx.AllocTempBufferView(vtxSize, 4); // TODO: Do we need 4 alignment?
		BufferView idxView = ctx.AllocTempBufferView(idxSize, 4);

		// Copy vertex and index buffer data to a single buffer
		ImDrawVert* vtxCPUMem = reinterpret_cast<ImDrawVert*>(vtxView.data);
		ImDrawIdx* idxCPUMem = reinterpret_cast<ImDrawIdx*>(idxView.data);
		for (int i = 0; i < drawData->CmdListsCount; ++i)
		{
			const ImDrawList* drawList = drawData->CmdLists[i];
			memcpy(vtxCPUMem, &drawList->VtxBuffer[0], drawList->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxCPUMem, &drawList->IdxBuffer[0], drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxCPUMem += drawList->VtxBuffer.Size;
			idxCPUMem += drawList->IdxBuffer.Size;
		}

		ctx.BeginRenderPass(m_Pipeline);
		{
			ctx.SetVertexBuffer(vtxView.buffer, vtxView.offset, sizeof(ImDrawVert));
			ctx.SetIndexBuffer(idxView.buffer, idxView.offset, Format::R16_UINT);
			const float4x4 mvp = ComputeProjectionMatrix(drawData);
			ctx.SetPushConstants(&mvp, sizeof(float4x4));
			ctx.SetShaderResource(m_FontTex, m_FontTexProxy);
			ctx.SetBlendFactor(float4(0));
			uint32 vtxOffset = 0, idxOffset = 0;
			ImVec2 clipOff = drawData->DisplayPos;
			for (int i = 0; i < drawData->CmdListsCount; ++i)
			{
				const ImDrawList* cmdList = drawData->CmdLists[i];
				for (int j = 0; j < cmdList->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmdList->CmdBuffer[j];
					if (pcmd->UserCallback != NULL)
					{
						if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
						{
							// TODO
						}
						else
						{
							pcmd->UserCallback(cmdList, pcmd);
						}
					}
					else
					{
						ImVec2 clipMin(pcmd->ClipRect.x - clipOff.x, pcmd->ClipRect.y - clipOff.y);
						ImVec2 clipMax(pcmd->ClipRect.z - clipOff.x, pcmd->ClipRect.w - clipOff.y);
						if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
						{
							continue;
						}

						ctx.SetScissorRect(int4(clipMin.x, clipMin.y, clipMax.x, clipMax.y));
						ctx.DrawIndexedInstanced(pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset, 0);
					}
				}
				idxOffset += cmdList->IdxBuffer.Size;
				vtxOffset += cmdList->VtxBuffer.Size;
			}
		}
		ctx.EndRenderPass();
	}
}
