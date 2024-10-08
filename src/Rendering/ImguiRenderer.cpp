#include "vastpch.h"
#include "Rendering/ImguiRenderer.h"
#include "Rendering/Imgui.h"

#include "Core/Window.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/GPUResourceManager.h"

namespace vast
{

	ImguiRenderer::ImguiRenderer(GraphicsContext& ctx, Window& window)
		: ctx(ctx)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		Event::Subscribe<WindowResizeEvent>("ImguiRenderer", VAST_EVENT_HANDLER(ImguiRenderer::OnWindowResizeEvent, WindowResizeEvent));

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		uint2 windowSize = window.GetSize();
		io.DisplaySize = ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));

		{
			// Create and upload font texture for default font atlas (see CreateContext()).
			unsigned char* texData;
			int32 texWidth, texHeight;
			io.Fonts->GetTexDataAsRGBA32(&texData, &texWidth, &texHeight);

			TextureDesc fontAtlasDesc =
			{
				.format = TexFormat::RGBA8_UNORM_SRGB,
				.width = static_cast<uint32>(texWidth),
				.height = static_cast<uint32>(texHeight),
				.viewFlags = TexViewFlags::SRV,
			};
			m_FontTex = rm.CreateTexture(fontAtlasDesc, texData);
			io.Fonts->SetTexID(&m_FontTex);
		}
		{
			// Create ImGui shaders and pipeline
			BlendState bs = BlendState::Preset::kAdditive;
			bs.dstBlendAlpha = Blend::INV_SRC_ALPHA;

			PipelineDesc pipelineDesc =
			{
				.vs = {.type = ShaderType::VERTEX, .shaderName = "Imgui.hlsl", .entryPoint = "VS_Imgui"},
				.ps = {.type = ShaderType::PIXEL,  .shaderName = "Imgui.hlsl", .entryPoint = "PS_Imgui"},
				.blendStates = { bs },
				.depthStencilState = DepthStencilState::Preset::kDisabled,
				.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
			};
			m_Pipeline = rm.CreatePipeline(pipelineDesc);
			m_CbvProxy = rm.LookupShaderResource(m_Pipeline, "ImguiCB");
		}

		ImGui::StyleColorsDark();
	}

	ImguiRenderer::~ImguiRenderer()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyTexture(m_FontTex);
		rm.DestroyPipeline(m_Pipeline);
	}

	void ImguiRenderer::BeginCommandRecording()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		ImGui::NewFrame();
	}

	void ImguiRenderer::EndCommandRecording()
	{
		VAST_PROFILE_TRACE_FUNCTION;

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
		VAST_PROFILE_TRACE_FUNCTION;

		VAST_ASSERTF(ctx.IsInFrame() && !ctx.IsInRenderPass(), "This function must be invoked within Graphics Context frame bounds and out of a Render Pass.");

		ImDrawData* drawData = ImGui::GetDrawData();

		if (!drawData || drawData->TotalVtxCount == 0)
			return;

		// Avoid rendering when minimized
		if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
			return;

		// Get memory for vertex and index buffers
		VAST_PROFILE_TRACE_BEGIN("Merge Buffers");
		const uint32 vtxSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
		const uint32 idxSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;

		GPUResourceManager& rm = ctx.GetGPUResourceManager();
		BufferView vtxView = rm.AllocTempBufferView(vtxSize, 4); // TODO: Do we need 4 alignment?
		BufferView idxView = rm.AllocTempBufferView(idxSize, 4);

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
		VAST_PROFILE_TRACE_END("Merge Buffers");

		ctx.BeginRenderPassToBackBuffer(m_Pipeline, LoadOp::LOAD, StoreOp::STORE);
		{
			ctx.BindVertexBuffer(vtxView.buffer, vtxView.offset, sizeof(ImDrawVert));
			ctx.BindIndexBuffer(idxView.buffer, idxView.offset, IndexBufFormat::R16_UINT);

			// Bind temporary CBV
			const float4x4 mvp = ComputeProjectionMatrix(drawData);
			BufferView cbv = rm.AllocTempBufferView(sizeof(float4x4), CONSTANT_BUFFER_ALIGNMENT);
			memcpy(cbv.data, &mvp, sizeof(float4x4));
			ctx.BindConstantBuffer(m_CbvProxy, cbv.buffer, cbv.offset);

			ctx.SetBlendFactor(float4(0));
			uint32 vtxOffset = 0, idxOffset = 0;
			ImVec2 clipOff = drawData->DisplayPos;
			VAST_PROFILE_TRACE_BEGIN("Draw Loop");
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
						VAST_ASSERT(h.IsValid());

						if (!rm.GetIsReady(h))
							continue;

						// TODO: Do we need explicit transitions here? If so we could bundle them in one go.
						// TODO: Support choosing texture filtering.
						// TODO: Support cubemap images.
						uint32 srvIndex = rm.GetBindlessSRV(h);
						ctx.SetPushConstants(&srvIndex, sizeof(uint32));
						ctx.SetScissorRect(int4(clipMin.x, clipMin.y, clipMax.x, clipMax.y));
						ctx.DrawIndexedInstanced(drawCmd->ElemCount, 1, drawCmd->IdxOffset + idxOffset, drawCmd->VtxOffset + vtxOffset, 0);
					}
				}
				idxOffset += drawList->IdxBuffer.Size;
				vtxOffset += drawList->VtxBuffer.Size;
			}
			VAST_PROFILE_TRACE_END("Draw Loop");
		}
		ctx.EndRenderPass();
	}

	void ImguiRenderer::OnWindowResizeEvent(const WindowResizeEvent& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(event.m_WindowSize.x), static_cast<float>(event.m_WindowSize.y));
	}

}
