#pragma once

#include "Core/core.h"
#include "Core/types.h"

namespace vast::gfx
{
	struct Resource; // TODO
	struct Texture; // TODO

	enum class Format
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
	};

	enum class ResourceState
	{
		RENDER_TARGET = 0x4, // TODO TEMP: Make this crossplatform-safe.
		PRESENT = 0,
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
		static Ptr<GraphicsContext> Create(const GraphicsParams& params = GraphicsParams());
		virtual ~GraphicsContext() = default;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Submit() = 0;
		virtual void Present() = 0;

		virtual Texture& GetCurrentBackBuffer() const = 0;

		virtual void AddBarrier(Resource& resource, const ResourceState& state) = 0;
		virtual void FlushBarriers() = 0;

		virtual void Reset() = 0;
		virtual void ClearRenderTarget(const Texture& texture, float4 color) = 0;
	};

}