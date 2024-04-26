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

	class ResourceManager;

	class GraphicsContext
	{
	public:
		GraphicsContext(const GraphicsParams& params = GraphicsParams());
		~GraphicsContext();

		void BeginFrame();
		void EndFrame();

		void BeginRenderPass(PipelineHandle h, const RenderPassTargets targets);
		// Renders to the back buffer as the only target
		void BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE);
		void EndRenderPass();

		bool IsInFrame() const;
		bool IsInRenderPass() const;

		// TODO: We may want to do Begin/End functions for compute to be analogous with graphics 
		// pipelines, despite it being perhaps redundant.
		void BindPipelineForCompute(PipelineHandle h);

		// Waits for the current batch of GPU work to finish.
		void WaitForIdle();
		// Waits for all active GPU work to finish as well as any queued resource destructions and 
		// shader reloads.
		void FlushGPU();

		// - GPU Resources --------------------------------------------------------------------- //

		BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr, const size_t dataSize = 0);
		TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr);
		PipelineHandle CreatePipeline(const PipelineDesc& desc);
		PipelineHandle CreatePipeline(const ShaderDesc& csDesc);

		void DestroyBuffer(BufferHandle h);
		void DestroyTexture(TextureHandle h);
		void DestroyPipeline(PipelineHandle h);

		void UpdateBuffer(BufferHandle h, void* data, const size_t size);

		void ReloadShaders(PipelineHandle h);

		ShaderResourceProxy LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName);

		// TODO: This should be separate from the Context (ContentLoader?)
		TextureHandle LoadTextureFromFile(const std::string& filePath, bool sRGB = true);

		// Returns a BufferView containing CPU-write/GPU-read memory that is alive for the duration of
		// the frame and automatically invalidated after that.
		BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0);

		void SetDebugName(BufferHandle h, const std::string& name);
		void SetDebugName(TextureHandle h, const std::string& name);

		const uint8* GetBufferData(BufferHandle h);

		// TODO: Review ready check for resources.
		bool GetIsReady(BufferHandle h);
		bool GetIsReady(TextureHandle h);

		TexFormat GetTextureFormat(TextureHandle h);

		// Resource Transitions
		void AddBarrier(BufferHandle h, ResourceState newState);
		void AddBarrier(TextureHandle h, ResourceState newState);
		void FlushBarriers();

		// Resource View Binding
		void BindVertexBuffer(BufferHandle h, uint32 offset = 0, uint32 stride = 0);
		void BindIndexBuffer(BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT);
		void BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset = 0);

		// Passes some data to be used in the GPU under a constant buffer declared in a shader using slot
		// 'PushConstantRegister'. Push Constants are best suited for small amounts of data that change
		// frequently (e.g. PerObjectSpace). Size parameter is expected in bytes.
		void SetPushConstants(const void* data, const uint32 size);

		void BindSRV(ShaderResourceProxy proxy, BufferHandle h);
		void BindSRV(ShaderResourceProxy proxy, TextureHandle h);
		void BindUAV(ShaderResourceProxy proxy, TextureHandle h, uint32 mipLevel = 0);

		// Query Bindless View Indices
		uint32 GetBindlessSRV(BufferHandle h);
		uint32 GetBindlessSRV(TextureHandle h);
		uint32 GetBindlessUAV(TextureHandle h, uint32 mipLevel = 0);

		// - Pipeline State -------------------------------------------------------------------- //

		void SetScissorRect(int4 rect);
		void SetBlendFactor(float4 blend);

		// - Draw/Dispatch Calls --------------------------------------------------------------- //

		void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0);
		void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0);
		void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0);
		void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation);
		void DrawFullscreenTriangle();

		void Dispatch(uint3 threadGroupCount);

		// - Swap Chain/Back Buffers ----------------------------------------------------------- //

		uint2 GetBackBufferSize() const;
		float GetBackBufferAspectRatio() const;
		TexFormat GetBackBufferFormat() const;

		// - Timestamps ------------------------------------------------------------------------ //

		uint32 BeginTimestamp();
		void EndTimestamp(uint32 timestampIdx);
		// Call after EndFrame() and before the next BeginFrame() to collect data on the frame that just ended.
		double GetTimestampDuration(uint32 timestampIdx);
		double GetLastFrameDuration();

	private:
		bool m_bHasFrameBegun = false;
		bool m_bHasRenderPassBegun = false;

		Ptr<ResourceManager> m_ResourceManager;

		double m_TimestampFrequency = 0.0;

		void CreateProfilingResources();
		void DestroyProfilingResources();
		void CollectTimestamps();

		uint32 m_TimestampCount = 0;
		uint32 m_GpuFrameTimestampIdx = 0;
		Array<BufferHandle, NUM_FRAMES_IN_FLIGHT> m_TimestampsReadbackBuf;
		Array<const uint64*, NUM_FRAMES_IN_FLIGHT> m_TimestampData;
	};
}