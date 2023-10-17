#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{

	class EnvBRDF
	{
	public:
		static void CreatePSOs(GraphicsContext& ctx);
		static void DestroyPSOs(GraphicsContext& ctx);

		struct Params
		{
			uint2 resolution = uint2(32);
			uint32 numSamples = 1024;
		};

		static TextureHandle GenerateLUT(GraphicsContext& ctx, const Params& p = Params());
	};

}
