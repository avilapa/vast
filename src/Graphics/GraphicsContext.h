#pragma once

#include "Core/Core.h"
#include "Core/Types.h"
#include "Graphics/Graphics.h"
#include "Graphics/ResourceHandles.h"

namespace vast::gfx
{
	using TextureHandle = Handle<Texture>;

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

		// TODO: Clear flags
		virtual void BeginRenderPass() = 0;
		virtual void BeginRenderPass(const TextureHandle& h) = 0;
		virtual void EndRenderPass() = 0;
	};

}