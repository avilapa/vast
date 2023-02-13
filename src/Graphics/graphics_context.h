#pragma once

#include "Core/core.h"
#include "Core/types.h"

namespace vast::gfx
{
	enum class Format
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
	};

	struct GraphicsParams
	{
		GraphicsParams() : swapChainSize(1600, 900), swapChainFormat(Format::RGBA8_UNORM) {}

		uint2 swapChainSize;
		Format swapChainFormat;
	};

	class GraphicsContext
	{
	public:
		static std::unique_ptr<GraphicsContext> Create(const GraphicsParams& params = GraphicsParams());

		virtual ~GraphicsContext() = default;
	};

}