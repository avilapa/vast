#pragma once

#include "Core/Core.h"
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

	// TODO: Look into __declspec(novtable)
	class GraphicsContext
	{
	public:
		static Ptr<GraphicsContext> Create(const GraphicsParams& params = GraphicsParams());
		virtual ~GraphicsContext() = default;

		// - GPU Resources --------------------------------------------------------------------- //
	public:
		BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, const size_t dataSize = 0);
		TextureHandle CreateTexture(const TextureDesc& desc, void* initialData = nullptr);
		PipelineHandle CreatePipeline(const PipelineDesc& h);

		void UpdateBuffer(BufferHandle h, void* data, const size_t size);
		void UpdatePipeline(PipelineHandle h);

		void DestroyBuffer(BufferHandle h);
		void DestroyTexture(TextureHandle h);
		void DestroyPipeline(PipelineHandle h);

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		virtual BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0) = 0;
		virtual ShaderResourceProxy LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName) = 0;
		TextureHandle LoadTextureFromFile(const std::string& filePath, bool sRGB = true);

	protected:
		Ptr<HandlePool<Buffer, NUM_BUFFERS>> m_BufferHandles = nullptr;
		Ptr<HandlePool<Texture, NUM_TEXTURES>> m_TextureHandles = nullptr;
		Ptr<HandlePool<Pipeline, NUM_PIPELINES>> m_PipelineHandles = nullptr;

		void CreateHandlePools();
		void DestroyHandlePools();

		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction = {};
		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction = {};
		Array<Vector<PipelineHandle>, NUM_FRAMES_IN_FLIGHT> m_PipelinesMarkedForDestruction = {};

		void ProcessDestructions(uint32 frameId);

		struct TempAllocator
		{
			BufferHandle buffer;
			uint32 size = 0;
			uint32 offset = 0;

			void Reset() { offset = 0; }
		};
		// TODO: Could this be replaced by a single, big dynamic buffer?
		Array<TempAllocator, NUM_FRAMES_IN_FLIGHT> m_TempFrameAllocators = {};

		void CreateTempFrameAllocators();
		void DestroyTempFrameAllocators();

		virtual void CreateBuffer_Internal(BufferHandle h, const BufferDesc& desc) = 0;
		virtual void UpdateBuffer_Internal(BufferHandle h, void* srcMem, size_t srcSize) = 0;
		virtual void DestroyBuffer_Internal(BufferHandle h) = 0;

		virtual void CreateTexture_Internal(TextureHandle h, const TextureDesc& desc) = 0;
		virtual void UpdateTexture_Internal(TextureHandle h, void* srcMem) = 0;
		virtual void DestroyTexture_Internal(TextureHandle h) = 0;

		virtual void CreatePipeline_Internal(PipelineHandle h, const PipelineDesc& desc) = 0;
		virtual void UpdatePipeline_Internal(PipelineHandle h) = 0;
		virtual void DestroyPipeline_Internal(PipelineHandle h) = 0;

		// - Render Commands ------------------------------------------------------------------- //
	public:
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual bool IsInFrame() const;
		// Waits for all active GPU work to finish as well as any queued resource destructions.
		virtual void FlushGPU() = 0;

		// Renders to the back buffer as the only target
		virtual void BeginRenderPassToBackBuffer(const PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE) = 0;
		virtual void BeginRenderPass(const PipelineHandle h, const RenderPassTargets targets) = 0;
		virtual void EndRenderPass() = 0;
		virtual bool IsInRenderPass() const;

		// Resource Binding
		virtual void SetVertexBuffer(const BufferHandle h, uint32 offset = 0, uint32 stride = 0) = 0;
		virtual void SetIndexBuffer(const BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT) = 0;
		virtual void SetConstantBufferView(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		virtual void SetShaderResourceView(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
		virtual void SetShaderResourceView(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy) = 0;
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

		virtual uint2 GetBackBufferSize() const = 0;
		virtual float GetBackBufferAspectRatio() const = 0;
		virtual TexFormat GetBackBufferFormat() const = 0;
		virtual TexFormat GetTextureFormat(const TextureHandle h) = 0;

		virtual uint32 GetBindlessIndex(const BufferHandle h) = 0;
		virtual uint32 GetBindlessIndex(const TextureHandle h) = 0;

		virtual bool GetIsReady(const BufferHandle h) = 0;
		virtual bool GetIsReady(const TextureHandle h) = 0;

	protected:
		uint32 m_FrameId = 0;

		bool m_bHasFrameBegun = false;
		bool m_bHasRenderPassBegun = false;
	};
}