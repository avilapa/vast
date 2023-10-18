#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{

	class EnvironmentBRDF
	{
	public:
		static void CreatePSOs(GraphicsContext& ctx);
		static void ReloadPSOs(GraphicsContext& ctx);
		static void DestroyPSOs(GraphicsContext& ctx);

		struct Params
		{
			uint32 resolution = 32;
			uint32 numSamples = 1024;
		};

		static TextureHandle GenerateLUT(GraphicsContext& ctx, const Params& p = Params());
	};

}
