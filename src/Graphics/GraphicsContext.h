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

	struct BufferView
	{
		ResourceHandle<Buffer> buffer;
		uint8* data;
		uint32 offset;
	};

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

		virtual void SetRenderTarget(const TextureHandle h) = 0;

		virtual void BeginRenderPass(const PipelineHandle h, ClearParams clear = ClearParams()) = 0;
		virtual void EndRenderPass() = 0;

		virtual void SetVertexBuffer(const BufferHandle h, uint32 offset = 0, uint32 stride = 0) = 0;
		virtual void SetIndexBuffer(const BufferHandle h, uint32 offset = 0, Format format = Format::UNKNOWN) = 0;
		virtual void SetShaderResource(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		virtual void SetShaderResource(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		virtual void SetPushConstants(const void* data, const uint32 size) = 0;

		virtual void SetScissorRect(int4 rect) = 0;
		virtual void SetBlendFactor(float4 blend) = 0;

		virtual void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0) = 0;
		virtual void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0) = 0;
		virtual void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0) = 0;
		virtual void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation) = 0;
		virtual void DrawFullscreenTriangle() = 0;

		virtual BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, const size_t dataSize = 0) = 0;
		virtual TextureHandle CreateTexture(const TextureDesc& desc, void* initialData = nullptr) = 0;
		virtual PipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;

		virtual void UpdateBuffer(const BufferHandle h, void* data, const size_t size) = 0;

		virtual void DestroyBuffer(const BufferHandle h) = 0;
		virtual void DestroyTexture(const TextureHandle h) = 0;
		virtual void DestroyPipeline(const PipelineHandle h) = 0;

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		virtual BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0) = 0;

		virtual ShaderResourceProxy LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName) = 0;

		virtual Format GetBackBufferFormat() const = 0;

		virtual uint32 GetBindlessIndex(const BufferHandle h) = 0;

		virtual uint32 GetBindlessIndex(const TextureHandle h) = 0;
		virtual Format GetTextureFormat(const TextureHandle h) = 0;

		virtual bool GetIsReady(const TextureHandle h) = 0;
	};
}