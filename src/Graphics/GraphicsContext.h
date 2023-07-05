#pragma once

#include "Core/Core.h"
#include "Core/Types.h"
#include "Core/Handles.h"
#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast::gfx
{
	struct GraphicsParams
	{
		GraphicsParams() : swapChainSize(1600, 900), swapChainFormat(TexFormat::RGBA8_UNORM), backBufferFormat(TexFormat::RGBA8_UNORM_SRGB) {}

		uint2 swapChainSize;
		TexFormat swapChainFormat;
		TexFormat backBufferFormat;
	};

	class GraphicsContext
	{
	public:
		static Ptr<GraphicsContext> Create(const GraphicsParams& params = GraphicsParams());

		virtual ~GraphicsContext() = default;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		// Dump Output RT to BackBuffer to allow UI elements to blend on top before EndFrame is called.
		virtual void RenderOutputToBackBuffer() = 0;
		// Waits for all active GPU work to finish as well as any queued resource destructions.
		virtual void FlushGPU() = 0;

		// Render Passes
		virtual void BeginRenderPassToBackBuffer(const PipelineHandle h) = 0;
		virtual void BeginRenderPass(const PipelineHandle h, const RenderPassTargets targets) = 0;
		virtual void EndRenderPass() = 0;

		// Resource Binding
		virtual void SetVertexBuffer(const BufferHandle h, uint32 offset = 0, uint32 stride = 0) = 0;
		virtual void SetIndexBuffer(const BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT) = 0;
		virtual void SetShaderResource(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		virtual void SetShaderResource(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		// Passes some data to be used in the GPU under a constant buffer declared in a shader using slot
		// 'PushConstantRegister'. Push Constants are best suited for small amounts of data that change
		// frequently (e.g. PerObjectSpace). Size parameter is expected in bytes.
		virtual void SetPushConstants(const void* data, const uint32 size) = 0;

		virtual void SetScissorRect(int4 rect) = 0;
		virtual void SetBlendFactor(float4 blend) = 0;

		virtual void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0) = 0;
		virtual void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0) = 0;
		virtual void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0) = 0;
		virtual void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation) = 0;
		virtual void DrawFullscreenTriangle() = 0;

		// Resource Creation/Destruction/Update
		virtual BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, const size_t dataSize = 0, const std::string& debugName = "Unnamed Buffer") = 0;
		virtual BufferHandle CreateBuffer(const BufferDesc& desc, const std::string& debugName) = 0;
		virtual TextureHandle CreateTexture(const TextureDesc& desc, void* initialData = nullptr, const std::string& debugName = "Unnamed Texture") = 0;
		virtual TextureHandle CreateTexture(const TextureDesc& desc, const std::string& debugName) = 0;
		virtual TextureHandle CreateTexture(const std::string& filePath, const std::string& debugName, bool sRGB = true) = 0;
		virtual TextureHandle CreateTexture(const std::string& filePath, bool sRGB = true) = 0;
		virtual PipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;

		virtual void UpdateBuffer(const BufferHandle h, void* data, const size_t size) = 0;
		virtual void UpdatePipeline(const PipelineHandle h) = 0;

		virtual void DestroyBuffer(const BufferHandle h) = 0;
		virtual void DestroyTexture(const TextureHandle h) = 0;
		virtual void DestroyPipeline(const PipelineHandle h) = 0;

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		virtual BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0) = 0;

		virtual ShaderResourceProxy LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName) = 0;

		virtual TextureHandle GetOutputRenderTarget() const = 0;
		virtual TexFormat GetOutputRenderTargetFormat() const = 0;
		virtual uint2 GetOutputRenderTargetSize() const = 0;

		virtual TexFormat GetTextureFormat(const TextureHandle h) = 0;

		virtual uint32 GetBindlessIndex(const BufferHandle h) = 0;
		virtual uint32 GetBindlessIndex(const TextureHandle h) = 0;
		virtual bool GetIsReady(const BufferHandle h) = 0;
		virtual bool GetIsReady(const TextureHandle h) = 0;
	};
}