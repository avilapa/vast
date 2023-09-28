#pragma once

#include "Core/Core.h"
#include "Graphics/Handles.h"
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

		void BeginFrame();
		void EndFrame();

		virtual bool IsInFrame() const;

		// - Render Pass ----------------------------------------------------------------------- //

		virtual void BeginRenderPass(PipelineHandle h, const RenderPassTargets targets) = 0;
		// Renders to the back buffer as the only target
		virtual void BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE) = 0;
		virtual void EndRenderPass() = 0;

		virtual bool IsInRenderPass() const;

		// TODO: We may want to do Begin/End functions for compute to be analogous with graphics 
		// pipelines, despite it being perhaps redundant.
		virtual void BindPipelineForCompute(PipelineHandle h) = 0;
		virtual void Dispatch(uint3 threadGroupCount) = 0;

		// - GPU Resources --------------------------------------------------------------------- //

		BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr, const size_t dataSize = 0);
		TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr);
		PipelineHandle CreatePipeline(const PipelineDesc& desc);
		PipelineHandle CreatePipeline(const ShaderDesc& csDesc);

		void UpdateBuffer(BufferHandle h, void* data, const size_t size);
		void UpdatePipeline(PipelineHandle h);

		void DestroyBuffer(BufferHandle h);
		void DestroyTexture(TextureHandle h);
		void DestroyPipeline(PipelineHandle h);

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0);
		virtual ShaderResourceProxy LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName) = 0;
		TextureHandle LoadTextureFromFile(const std::string& filePath, bool sRGB = true);

		virtual void AddBarrier(BufferHandle h, ResourceState newState) = 0;
		virtual void AddBarrier(TextureHandle h, ResourceState newState) = 0;
		virtual void FlushBarriers() = 0;
		
		// - Resource Binding ------------------------------------------------------------------ //

		virtual void BindVertexBuffer(BufferHandle h, uint32 offset = 0, uint32 stride = 0) = 0;
		virtual void BindIndexBuffer(BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT) = 0;
		virtual void BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset = 0) = 0;
		virtual void BindSRV(ShaderResourceProxy proxy, BufferHandle h) = 0;
		virtual void BindSRV(ShaderResourceProxy proxy, TextureHandle h) = 0;
		virtual void BindUAV(ShaderResourceProxy proxy, TextureHandle h) = 0;
		// Passes some data to be used in the GPU under a constant buffer declared in a shader using slot
		// 'PushConstantRegister'. Push Constants are best suited for small amounts of data that change
		// frequently (e.g. PerObjectSpace). Size parameter is expected in bytes.
		virtual void SetPushConstants(const void* data, const uint32 size) = 0;

		// - Pipeline State -------------------------------------------------------------------- //

		virtual void SetScissorRect(int4 rect) = 0;
		virtual void SetBlendFactor(float4 blend) = 0;

		// - Draw Calls ------------------------------------------------------------------------ //

		virtual void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0) = 0;
		virtual void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0) = 0;
		virtual void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0) = 0;
		virtual void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation) = 0;
		virtual void DrawFullscreenTriangle() = 0;

		// - Misc ------------------------------------------------------------------------------ //

		virtual uint2 GetBackBufferSize() const = 0;
		virtual float GetBackBufferAspectRatio() const = 0;
		virtual TexFormat GetBackBufferFormat() const = 0;
		virtual TexFormat GetTextureFormat(TextureHandle h) = 0;

		virtual uint32 GetBindlessSRV(BufferHandle h) = 0;
		virtual uint32 GetBindlessSRV(TextureHandle h) = 0;
		virtual uint32 GetBindlessUAV(TextureHandle h) = 0;

		// TODO: Review ready check for resources.
		virtual bool GetIsReady(BufferHandle h) = 0;
		virtual bool GetIsReady(TextureHandle h) = 0;

		virtual const uint8* GetBufferData(BufferHandle h) = 0;

		// Waits for all active GPU work to finish as well as any queued resource destructions.
		virtual void FlushGPU() = 0;

		uint32 BeginTimestamp();
		void EndTimestamp(uint32 timestampIdx);
		// Call after EndFrame() and before the next BeginFrame() to collect data on the frame that just ended.
		double GetTimestampDuration(uint32 timestampIdx);
		double GetLastFrameDuration();

	protected:
		uint32 m_FrameId = 0;

		bool m_bHasRenderPassBegun = false;

		virtual void BeginFrame_Internal() = 0;
		virtual void EndFrame_Internal() = 0;

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
		virtual void UpdateBuffer_Internal(BufferHandle h, const void* srcMem, size_t srcSize) = 0;
		virtual void DestroyBuffer_Internal(BufferHandle h) = 0;

		virtual void CreateTexture_Internal(TextureHandle h, const TextureDesc& desc) = 0;
		virtual void UpdateTexture_Internal(TextureHandle h, const void* srcMem) = 0;
		virtual void DestroyTexture_Internal(TextureHandle h) = 0;

		virtual void CreatePipeline_Internal(PipelineHandle h, const PipelineDesc& desc) = 0;
		virtual void CreatePipeline_Internal(PipelineHandle h, const ShaderDesc& csDesc) = 0;
		virtual void UpdatePipeline_Internal(PipelineHandle h) = 0;
		virtual void DestroyPipeline_Internal(PipelineHandle h) = 0;

		double m_TimestampFrequency = 0.0;

		void CreateProfilingResources();
		void DestroyProfilingResources();
		void CollectTimestamps();

		virtual void BeginTimestamp_Internal(uint32 idx) = 0;
		virtual void EndTimestamp_Internal(uint32 idx) = 0;
		virtual void CollectTimestamps_Internal(BufferHandle h, uint32 count) = 0;

	private:
		bool m_bHasFrameBegun = false;

		uint32 m_TimestampCount = 0;
		uint32 m_GpuFrameTimestampIdx = 0;
		Array<BufferHandle, NUM_FRAMES_IN_FLIGHT> m_TimestampsReadbackBuf;
		Array<const uint64*, NUM_FRAMES_IN_FLIGHT> m_TimestampData;

	};
}