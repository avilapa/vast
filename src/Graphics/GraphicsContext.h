#pragma once

#include "Core/Core.h"
#include "Core/Types.h"
#include "Graphics/Graphics.h"
#include "Graphics/ResourceHandles.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast::gfx
{
	class Buffer;
	class Texture;
	class Pipeline;

	using BufferHandle = ResourceHandle<Buffer>;
	using TextureHandle	= ResourceHandle<Texture>;
	using PipelineHandle = ResourceHandle<Pipeline>;

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
		virtual void SetRenderTarget(const TextureHandle h) = 0;
		virtual void SetVertexBuffer(const BufferHandle h) = 0;
		virtual void SetIndexBuffer(const BufferHandle h) = 0;
		virtual void SetShaderResource(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		virtual void SetPushConstants(const void* data, const size_t size) = 0;
		virtual void BeginRenderPass(const PipelineHandle h) = 0;
		virtual void BeginRenderPass(const PipelineHandle h, ClearParams clear = ClearParams()) = 0;
		virtual void EndRenderPass() = 0;

		virtual void Draw(const uint32 vtxCount, const uint32 vtxStartLocation = 0) = 0;

		virtual BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, const size_t dataSize = 0) = 0;
		virtual TextureHandle CreateTexture(const TextureDesc& desc, void* initialData = nullptr) = 0;
		virtual PipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;

		virtual void UpdateBuffer(const BufferHandle h, void* data, const size_t size) = 0;

		virtual void DestroyBuffer(const BufferHandle h) = 0;
		virtual void DestroyTexture(const TextureHandle h) = 0;
		virtual void DestroyPipeline(const PipelineHandle h) = 0;

		virtual ShaderResourceProxy LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName) = 0;

		virtual Format GetBackBufferFormat() const = 0;
		virtual uint32 GetBindlessIndex(const BufferHandle h) = 0;
		virtual bool GetIsReady(const TextureHandle h) = 0;
	};

}