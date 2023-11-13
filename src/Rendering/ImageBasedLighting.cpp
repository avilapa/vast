#include "vastpch.h"
#include "Rendering/ImageBasedLighting.h"

namespace vast::gfx
{
	static PipelineHandle s_IntegrateIrradiancePso;
	static PipelineHandle s_PrefilterEnvironmentPso;

	static void OnReloadShadersEvent(const ReloadShadersEvent& event)
	{
		ImageBasedLighting::ReloadPSOs(event.ctx);
	}

	void ImageBasedLighting::CreatePSOs(GraphicsContext& ctx)
	{
		VAST_ASSERTF(!s_IntegrateIrradiancePso.IsValid() && !s_PrefilterEnvironmentPso.IsValid(), 
			"Cannot create Image Based Lighting PSOs, already initialized.");

		ShaderDesc csDesc =
		{
			.type = ShaderType::COMPUTE,
			.shaderName = "ImageBasedLighting.hlsl",
			.entryPoint = "CS_IntegrateDiffuseIrradiance",
		};
		s_IntegrateIrradiancePso = ctx.CreatePipeline(csDesc);
		csDesc.entryPoint = "CS_IntegratePrefilteredRadiance";
		s_PrefilterEnvironmentPso = ctx.CreatePipeline(csDesc);

		VAST_SUBSCRIBE_TO_EVENT("ImageBasedLighting", ReloadShadersEvent, VAST_EVENT_HANDLER_CB_STATIC(OnReloadShadersEvent, ReloadShadersEvent));
	}

	void ImageBasedLighting::ReloadPSOs(GraphicsContext& ctx)
	{
		VAST_ASSERTF(s_IntegrateIrradiancePso.IsValid() || s_PrefilterEnvironmentPso.IsValid(),
			"Cannot reload Image Based Lighting PSOs, not currently initialized.");

		ctx.ReloadShaders(s_IntegrateIrradiancePso);
		ctx.ReloadShaders(s_PrefilterEnvironmentPso);
	}
	
	void ImageBasedLighting::DestroyPSOs(GraphicsContext& ctx)
	{
		VAST_ASSERTF(s_IntegrateIrradiancePso.IsValid() || s_PrefilterEnvironmentPso.IsValid(), 
			"Cannot destroy Image Based Lighting PSOs, not currently initialized.");

		ctx.DestroyPipeline(s_IntegrateIrradiancePso);
		ctx.DestroyPipeline(s_PrefilterEnvironmentPso);
		s_IntegrateIrradiancePso = PipelineHandle();
		s_PrefilterEnvironmentPso = PipelineHandle();

		VAST_UNSUBSCRIBE_FROM_EVENT("ImageBasedLighting", ReloadShadersEvent);
	}

	TextureHandle ImageBasedLighting::GenerateIrradianceMap(GraphicsContext& ctx, TextureHandle skyboxMap, const IrradianceParams& p /* = Params() */)
	{
		// If PSOs were not explicitly created, create them on the fly and destroy them at the end of this function.
		bool bUseTransientPSO = !s_IntegrateIrradiancePso.IsValid();
		if (bUseTransientPSO)
		{
			VAST_LOG_WARNING("[renderer] ImageBasedLighting::GenerateIrradianceMap() call will create and destroy PSOs on the fly (if this is not intended, use explicit CreatePSOs/DestroyPSOs functions).");
			CreatePSOs(ctx);
		}

		TextureDesc textureDesc =
		{
			.type = TexType::TEXTURE_2D,
			.format = TexFormat::RGBA16_FLOAT,
			.width = p.resolution,
			.height = p.resolution,
			.depthOrArraySize = 6,
			.viewFlags = TexViewFlags::SRV | TexViewFlags::UAV,
			.name = "Irradiance Map"
		};

		TextureHandle irradianceMap = ctx.CreateTexture(textureDesc);
		VAST_ASSERT(irradianceMap.IsValid());

		{
			VAST_PROFILE_GPU_SCOPE("Integrate Irradiance Map (CS)", &ctx);
			ctx.BindPipelineForCompute(s_IntegrateIrradiancePso);

			ctx.AddBarrier(irradianceMap, ResourceState::UNORDERED_ACCESS);
			ctx.FlushBarriers();

			struct IrradianceIBL_Constants
			{
				uint32 SkyboxSrvIdx;
				uint32 IrradianceUavIdx;
				uint32 NumSamples;
			} pc = { ctx.GetBindlessSRV(skyboxMap), ctx.GetBindlessUAV(irradianceMap), p.numSamples };
			ctx.SetPushConstants(&pc, sizeof(IrradianceIBL_Constants));

			ctx.Dispatch(uint3(textureDesc.width / 32, textureDesc.height / 32, 6));

			ctx.AddBarrier(irradianceMap, ResourceState::PIXEL_SHADER_RESOURCE);
			ctx.FlushBarriers();
		}

		if (bUseTransientPSO)
		{
			// TODO BUG: There's a race condition using the transient PSO option while destroying PSOs.
			DestroyPSOs(ctx);
		}

		return irradianceMap;
	}

	TextureHandle ImageBasedLighting::GeneratePrefilteredEnvironmentMap(GraphicsContext& ctx, TextureHandle skyboxMap, const PrefilterParams& p /* = PrefilterParams() */)
	{
		// If PSOs were not explicitly created, create them on the fly and destroy them at the end of this function.
		bool bUseTransientPSO = !s_PrefilterEnvironmentPso.IsValid();
		if (bUseTransientPSO)
		{
			VAST_LOG_WARNING("[renderer] ImageBasedLighting::GeneratePrefilteredEnvironmentMap() call will create and destroy PSOs on the fly (if this is not intended, use explicit CreatePSOs/DestroyPSOs functions).");
			CreatePSOs(ctx);
		}

		TextureDesc textureDesc =
		{
			.type = TexType::TEXTURE_2D,
			.format = TexFormat::RGBA16_FLOAT,
			.width = p.resolution,
			.height = p.resolution,
			.depthOrArraySize = 6,
			.mipCount = p.numMipMaps,
			.viewFlags = TexViewFlags::SRV | TexViewFlags::UAV,
			.name = "Prefiltered Environment Map"
		};

		TextureHandle prefilteredEnvironmentMap = ctx.CreateTexture(textureDesc);
		VAST_ASSERT(prefilteredEnvironmentMap.IsValid());

		{
			VAST_PROFILE_GPU_SCOPE("Prefilter Environment Map (CS)", &ctx);
			ctx.BindPipelineForCompute(s_PrefilterEnvironmentPso);

			ctx.AddBarrier(prefilteredEnvironmentMap, ResourceState::UNORDERED_ACCESS);
			ctx.FlushBarriers();

			// TODO: Can we do all mips in a single compute pass? (do we want to?)
			for (uint32 i = 0, mipSize = p.resolution; i < p.numMipMaps; ++i, mipSize /= 2)
			{
				struct PrefilterIBL_Constants
				{
					uint32 SkyboxSrvIdx;
					uint32 PrefilteredEnvironmentUavIdx;
					uint32 NumSamples;
					uint32 NumMips;
					uint32 MipLevel;
				} pc = { ctx.GetBindlessSRV(skyboxMap), ctx.GetBindlessUAV(prefilteredEnvironmentMap, i), p.numSamples, p.numMipMaps, i };
				ctx.SetPushConstants(&pc, sizeof(PrefilterIBL_Constants));

				ctx.Dispatch(uint3(mipSize / 8, mipSize / 8, 6));
			}

			ctx.AddBarrier(prefilteredEnvironmentMap, ResourceState::PIXEL_SHADER_RESOURCE);
			ctx.FlushBarriers();
		}

		if (bUseTransientPSO)
		{
			// TODO BUG: There's a race condition using the transient PSO option while destroying PSOs.
			DestroyPSOs(ctx);
		}

		return prefilteredEnvironmentMap;
	}
}
