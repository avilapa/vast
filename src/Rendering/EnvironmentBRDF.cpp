#include "vastpch.h"
#include "Rendering/EnvironmentBRDF.h"

namespace vast::gfx
{

	static PipelineHandle s_IntegrateBrdfPso;

	void EnvironmentBRDF::CreatePSOs(GraphicsContext& ctx)
	{
		VAST_ASSERTF(!s_IntegrateBrdfPso.IsValid(), "Cannot create Environment BRDF PSO, already initialized.");

		ShaderDesc csDesc =
		{
			.type = ShaderType::COMPUTE,
			.shaderName = "EnvironmentBRDF.hlsl",
			.entryPoint = "CS_IntegrateBRDF",
		};
		s_IntegrateBrdfPso = ctx.CreatePipeline(csDesc);
	}

	void EnvironmentBRDF::ReloadPSOs(GraphicsContext& ctx)
	{
		VAST_ASSERTF(s_IntegrateBrdfPso.IsValid(), "Cannot reload Environment BRDF PSO, not currently initialized.");

		ctx.UpdatePipeline(s_IntegrateBrdfPso);
	}
	
	void EnvironmentBRDF::DestroyPSOs(GraphicsContext& ctx)
	{
		VAST_ASSERTF(s_IntegrateBrdfPso.IsValid(), "Cannot destroy Environment BRDF PSO, not currently initialized.");

		ctx.DestroyPipeline(s_IntegrateBrdfPso);
		s_IntegrateBrdfPso = PipelineHandle();
	}

	TextureHandle EnvironmentBRDF::GenerateLUT(GraphicsContext& ctx, const Params& p /* = Params() */)
	{
		// If PSOs were not explicitly created, create them on the fly and destroy them at the end of this function.
		bool bUseTransientPSO = !s_IntegrateBrdfPso.IsValid();
		if (bUseTransientPSO)
		{
			VAST_LOG_WARNING("[renderer] EnvBRDF::GenerateLUT() call will create and destroy PSOs on the fly (if this is not intended, use explicit CreatePSOs/DestroyPSOs functions).");
			CreatePSOs(ctx);
		}

		TextureDesc textureDesc =
		{
			.type = TexType::TEXTURE_2D,
			.format = TexFormat::RG16_FLOAT,
			.width = p.resolution,
			.height = p.resolution,
			.viewFlags = TexViewFlags::SRV | TexViewFlags::UAV,
			.name = "Environment BRDF LUT",
		};

		TextureHandle envBrdfLut = ctx.CreateTexture(textureDesc);
		VAST_ASSERT(envBrdfLut.IsValid());

		{
			VAST_PROFILE_GPU_SCOPE("Integrate Environment BRDF (CS)", &ctx);
			ctx.BindPipelineForCompute(s_IntegrateBrdfPso);

			ctx.AddBarrier(envBrdfLut, ResourceState::UNORDERED_ACCESS);
			ctx.FlushBarriers();

			struct EnvBRDF_Constants
			{
				uint32 EnvBrdfUavIdx;
				uint32 NumSamples;
			} pc = { ctx.GetBindlessUAV(envBrdfLut), p.numSamples};
			ctx.SetPushConstants(&pc, sizeof(EnvBRDF_Constants));

			ctx.Dispatch(uint3(p.resolution / 32, p.resolution / 32, 1));

			ctx.AddBarrier(envBrdfLut, ResourceState::PIXEL_SHADER_RESOURCE);
			ctx.FlushBarriers();
		}

		if (bUseTransientPSO)
		{
			// TODO BUG: There's a race condition using the transient PSO option while destroying PSOs.
			DestroyPSOs(ctx);
		}

		return envBrdfLut;
	}

}
