#pragma once

#include "Core/Core.h"
#include "Core/Types.h"
#include "Graphics/Graphics.h"

namespace vast::gfx
{
	struct GraphicsParams
	{
		GraphicsParams() : swapChainSize(1600, 900), swapChainFormat(Format::RGBA8_UNORM), backBufferFormat(Format::RGBA8_UNORM_SRGB) {}

		uint2 swapChainSize;
		Format swapChainFormat;
		Format backBufferFormat;
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

		virtual Texture GetCurrentBackBuffer() const = 0;

		virtual void AddBarrier(GPUResource& resource, const ResourceState& state) = 0;
		virtual void FlushBarriers() = 0;

		virtual void Reset() = 0;
		virtual void ClearRenderTarget(const Texture& texture, float4 color) = 0;
	};

}