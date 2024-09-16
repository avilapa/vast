#include "vastpch.h"
#include "Rendering/Skybox.h"
#include "Rendering/Shapes.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/GPUResourceManager.h"

namespace vast
{

	Skybox::Skybox(GraphicsContext& ctx, TexFormat rtFormat, TexFormat dsFormat, bool bUseReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
		: ctx(ctx)
		, m_bUsingReverseZ(bUseReverseZ)
	{
		auto dsState = DepthStencilState::Preset::kEnabledReadOnly;
		if (m_bUsingReverseZ != VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z)
		{
			dsState.depthFunc = GetFixedCompareFunc(CompareFunc::LESS_EQUAL, m_bUsingReverseZ);
		}

		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		m_PSO = rm.CreatePipeline(PipelineDesc{
			.vs = AllocVertexShaderDesc("Skybox.hlsl"),
			.ps = AllocPixelShaderDesc("Skybox.hlsl"),
			.depthStencilState = dsState,
			.rasterizerState = {.cullMode = CullMode::BACK },
			.renderPassLayout = {.rtFormats = { rtFormat }, .dsFormat = { dsFormat } }, 
		});

		auto vtxBufDesc = AllocVertexBufferDesc(sizeof(Cube::s_VerticesIndexed_Pos), sizeof(Cube::s_VerticesIndexed_Pos[0]));
		m_CubeVtxBuf = rm.CreateBuffer(vtxBufDesc, &Cube::s_VerticesIndexed_Pos, sizeof(Cube::s_VerticesIndexed_Pos));

		uint32 numIndices = static_cast<uint32>(Cube::s_Indices.size());
		auto idxBufDesc = AllocIndexBufferDesc(numIndices);
		m_CubeIdxBuf = rm.CreateBuffer(idxBufDesc, &Cube::s_Indices, numIndices * sizeof(uint16));
	}

	Skybox::~Skybox()
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		rm.DestroyPipeline(m_PSO);
		rm.DestroyBuffer(m_CubeVtxBuf);
		rm.DestroyBuffer(m_CubeIdxBuf);
	}

	void Skybox::Render(TextureHandle environmentMap, const RenderTargetDesc& rt, const RenderTargetDesc& ds, const PerspectiveCamera& camera)
	{
		VAST_ASSERTF(camera.GetIsReverseZ() == m_bUsingReverseZ, "Mismatched depth settings.");
		Render(environmentMap, rt, ds, camera.GetViewMatrix(), camera.GetProjectionMatrix());
	}

	void Skybox::Render(TextureHandle environmentMap, const RenderTargetDesc& rt, const RenderTargetDesc& ds, const float4x4& viewMatrix, const float4x4& projMatrix)
	{
		GPUResourceManager& rm = ctx.GetGPUResourceManager();

		if (!rm.GetIsReady(m_CubeVtxBuf) || !rm.GetIsReady(m_CubeIdxBuf) || !rm.GetIsReady(environmentMap))
			return;

		// TODO: If the user calls this function directly we don't currently check for ReverseZ settings.
		ctx.BeginRenderPass(m_PSO, RenderPassDesc{ .rt = { rt }, .ds = ds });
		{
			struct SkyboxCB
			{
				float4x4 view;
				float4x4 proj;
				uint32 texIdx;
				uint32 bUseReverseZ;
			} pc{viewMatrix, projMatrix, rm.GetBindlessSRV(environmentMap), m_bUsingReverseZ };

			ctx.SetPushConstants(&pc, sizeof(SkyboxCB));

			ctx.BindVertexBuffer(m_CubeVtxBuf);
			ctx.BindIndexBuffer(m_CubeIdxBuf);
			ctx.DrawIndexed(static_cast<uint32>(Cube::s_Indices.size()));
		}
		ctx.EndRenderPass();
	}

}
