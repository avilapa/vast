#pragma once

namespace vast::gfx
{

	enum class Format
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
		COUNT,
	};

	enum class ResourceState
	{
		RENDER_TARGET,
		PRESENT,
	};

}