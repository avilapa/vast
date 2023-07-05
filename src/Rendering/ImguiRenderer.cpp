#include "vastpch.h"
#include "Rendering/ImguiRenderer.h"

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
		VAST_PROFILE_SCOPE("ui", "Create Imgui Renderer");
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Init(windowHandle);
#endif
		{
			BlendState bs = BlendState::Preset::kAdditive;
			bs.dstBlendAlpha = Blend::INV_SRC_ALPHA;

			PipelineDesc pipelineDesc =
			{
				.vs = {.type = ShaderType::VERTEX, .shaderName = "imgui.hlsl", .entryPoint = "VS_Main"},
				.ps = {.type = ShaderType::PIXEL,  .shaderName = "imgui.hlsl", .entryPoint = "PS_Main"},
				.blendStates = { bs },
				.depthStencilState = DepthStencilState::Preset::kDisabled,
				.renderPassLayout = { .rtFormats = { ctx.GetOutputRenderTargetFormat() } },
			};
			m_Pipeline = ctx.CreatePipeline(pipelineDesc);
		}

		{
			ImGuiIO& io = ImGui::GetIO();
			unsigned char* texData;
			int32 texWidth, texHeight;
			io.Fonts->GetTexDataAsRGBA32(&texData, &texWidth, &texHeight);

			TextureDesc texDesc =
			{
				.format = TexFormat::RGBA8_UNORM_SRGB,
				.width  = static_cast<uint32>(texWidth),
				.height = static_cast<uint32>(texHeight),
				.viewFlags = TexViewFlags::SRV,
			};
			m_FontTex = ctx.CreateTexture(texDesc, texData);

			m_FontTexProxy = ctx.LookupShaderResource(m_Pipeline, "FontTexture");
		}
	}

	ImguiRenderer::~ImguiRenderer()
	{
		VAST_PROFILE_SCOPE("ui", "Destroy Imgui Renderer");
		ctx.DestroyTexture(m_FontTex);
		ctx.DestroyPipeline(m_Pipeline);

#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_Shutdown();
#endif
	}

	void ImguiRenderer::BeginFrame()
	{
		VAST_PROFILE_SCOPE("ui", "Begin Frame (UI)");
#if USE_IMGUI_STOCK_IMPL_WIN32
		ImGui_ImplWin32_NewFrame();
#endif
		{
			VAST_PROFILE_SCOPE("ui", "Imgui New Frame");
			ImGui::NewFrame();
		}
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
		VAST_PROFILE_SCOPE("ui", "End Frame (UI)");
		{
			VAST_PROFILE_SCOPE("ui", "Imgui Render");
			ImGui::Render();
		}

		ImDrawData* drawData = ImGui::GetDrawData();

		if (!drawData || drawData->TotalVtxCount == 0)
			return;

		// Avoid rendering when minimized
		if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
			return;

		if (!ctx.GetIsReady(m_FontTex))
			return;

		// Get memory for vertex and index buffers
		VAST_PROFILE_BEGIN("ui", "Merge Buffers");
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
		VAST_PROFILE_END("ui", "Merge Buffers");

		ctx.BeginRenderPassToBackBuffer(m_Pipeline);
		{
			ctx.SetVertexBuffer(vtxView.buffer, vtxView.offset, sizeof(ImDrawVert));
			ctx.SetIndexBuffer(idxView.buffer, idxView.offset, IndexBufFormat::R16_UINT);
			const float4x4 mvp = ComputeProjectionMatrix(drawData);
			ctx.SetPushConstants(&mvp, sizeof(float4x4));
			ctx.SetShaderResource(m_FontTex, m_FontTexProxy);
			ctx.SetBlendFactor(float4(0));
			uint32 vtxOffset = 0, idxOffset = 0;
			ImVec2 clipOff = drawData->DisplayPos;
			VAST_PROFILE_BEGIN("ui", "Draw Loop");
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

						ctx.SetScissorRect(int4(clipMin.x, clipMin.y, clipMax.x, clipMax.y));
						ctx.DrawIndexedInstanced(drawCmd->ElemCount, 1, drawCmd->IdxOffset + idxOffset, drawCmd->VtxOffset + vtxOffset, 0);
					}
				}
				idxOffset += drawList->IdxBuffer.Size;
				vtxOffset += drawList->VtxBuffer.Size;
			}
			VAST_PROFILE_END("ui", "Draw Loop");
		}
		ctx.EndRenderPass();
	}

}
