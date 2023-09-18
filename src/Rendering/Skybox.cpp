#include "vastpch.h"
#include "Rendering/Skybox.h"

namespace vast::gfx
{
	static const Array<s_float3, 8> s_CubeVertexData =
	{ {
		{-1.0f,  1.0f,  1.0f },
		{ 1.0f,  1.0f,  1.0f },
		{-1.0f, -1.0f,  1.0f },
		{ 1.0f, -1.0f,  1.0f },
		{-1.0f,  1.0f, -1.0f },
		{ 1.0f,  1.0f, -1.0f },
		{-1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
	} };

	static const Array<uint16, 36> s_CubeIndexData =
	{ {
		0, 1, 2,
		1, 3, 2,
		4, 6, 5,
		5, 6, 7,
		0, 2, 4,
		4, 2, 6,
		1, 5, 3,
		5, 7, 3,
		0, 4, 1,
		4, 5, 1,
		2, 3, 6,
		6, 3, 7,
	} };

	Skybox::Skybox(gfx::GraphicsContext& ctx_, TexFormat rtFormat, TexFormat dsFormat, bool bUseReverseZ /* = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z */)
		: ctx(ctx_)
		, m_bUsingReverseZ(bUseReverseZ)
	{
		auto dsState = DepthStencilState::Preset::kEnabledReadOnly;
		if (m_bUsingReverseZ != VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z)
		{
			dsState.depthFunc = GetFixedCompareFunc(CompareFunc::LESS_EQUAL, m_bUsingReverseZ);
		}

		m_PSO = ctx.CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "skybox.hlsl", .entryPoint = "VS_Cube"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "skybox.hlsl", .entryPoint = "PS_Cube"},
			.depthStencilState = dsState,
			.rasterizerState = {.cullMode = CullMode::BACK },
			.renderPassLayout = {.rtFormats = { rtFormat }, .dsFormat = { dsFormat } }, 
		});

		auto vtxBufDesc = AllocVertexBufferDesc(sizeof(s_CubeVertexData), sizeof(s_CubeVertexData[0]));
		m_CubeVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_CubeVertexData, sizeof(s_CubeVertexData));

		uint32 numIndices = static_cast<uint32>(s_CubeIndexData.size());
		auto idxBufDesc = AllocIndexBufferDesc(numIndices);
		m_CubeIdxBuf = ctx.CreateBuffer(idxBufDesc, &s_CubeIndexData, numIndices * sizeof(uint16));
	}

	Skybox::~Skybox()
	{
		ctx.DestroyPipeline(m_PSO);
		ctx.DestroyBuffer(m_CubeVtxBuf);
		ctx.DestroyBuffer(m_CubeIdxBuf);
	}

	void Skybox::Render(TextureHandle environmentMap, const RenderTargetDesc& rt, const RenderTargetDesc& ds, const PerspectiveCamera& camera)
	{
		VAST_ASSERTF(camera.GetIsReverseZ() == m_bUsingReverseZ, "Mismatched depth settings.");
		Render(environmentMap, rt, ds, camera.GetViewMatrix(), camera.GetProjectionMatrix());
	}

	void Skybox::Render(TextureHandle environmentMap, const RenderTargetDesc& rt, const RenderTargetDesc& ds, const float4x4& viewMatrix, const float4x4& projMatrix)
	{
		// TODO: If the user calls this function directly we don't currently check for ReverseZ settings.
		ctx.BeginRenderPass(m_PSO, RenderPassTargets{.rt = { rt }, .ds = ds });
		{
			if (ctx.GetIsReady(m_CubeVtxBuf) && ctx.GetIsReady(m_CubeIdxBuf) && ctx.GetIsReady(environmentMap))
			{
				struct SkyboxCB
				{
					float4x4 view;
					float4x4 proj;
					uint32 texIdx;
					uint32 bUseReverseZ;
				} pc{viewMatrix, projMatrix, ctx.GetBindlessIndex(environmentMap), m_bUsingReverseZ };

				ctx.SetPushConstants(&pc, sizeof(SkyboxCB));

				ctx.SetVertexBuffer(m_CubeVtxBuf);
				ctx.SetIndexBuffer(m_CubeIdxBuf);
				ctx.DrawIndexed(static_cast<uint32>(s_CubeIndexData.size()));
			}
		}
		ctx.EndRenderPass();
	}

}
