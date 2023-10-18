#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{

	class ImageBasedLighting
	{
	public:
		static void CreatePSOs(GraphicsContext& ctx);
		static void ReloadPSOs(GraphicsContext& ctx);
		static void DestroyPSOs(GraphicsContext& ctx);

		struct IrradianceParams
		{
			uint32 resolution = 64;
			uint32 numSamples = 16384;
		};

		struct PrefilterParams
		{
			uint32 resolution = 1024;
			uint32 numMipMaps = 7;
			uint32 numSamples = 2048;
		};

		static TextureHandle GenerateIrradianceMap(GraphicsContext& ctx, TextureHandle skyboxMap, const IrradianceParams& p = IrradianceParams());
		static TextureHandle GeneratePrefilteredEnvironmentMap(GraphicsContext& ctx, TextureHandle skyboxMap, const PrefilterParams& p = PrefilterParams());
	};

}
