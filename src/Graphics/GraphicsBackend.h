#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{
	enum class QueueType
	{
		GRAPHICS = 0,
		// TODO: Async Compute
		UPLOAD,
		COUNT,
	};

	// Note: We could use pimpl to avoid global state.
	class GraphicsBackend
	{
	public:
		static void Init(const GraphicsParams& params);
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();
		static uint32 GetFrameId();

		static void BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE);
		static void BeginRenderPass(PipelineHandle h, RenderPassTargets targets);
		static void EndRenderPass();

		static void BindPipelineForCompute(PipelineHandle h);

		static void WaitForIdle();

		// - GPU Resources --------------------------------------------------------------------- //

		static void CreateBuffer(BufferHandle h, const BufferDesc& desc);
		static void CreateTexture(TextureHandle h, const TextureDesc& desc);
		static void CreatePipeline(PipelineHandle h, const PipelineDesc& desc);
		static void CreatePipeline(PipelineHandle h, const ShaderDesc& csDesc);

		static void DestroyBuffer(BufferHandle h);
		static void DestroyTexture(TextureHandle h);
		static void DestroyPipeline(PipelineHandle h);

		static void UpdateBuffer(BufferHandle h, const void* srcMem, size_t srcSize);
		static void UpdateTexture(TextureHandle h, const void* srcMem);

		static void ReloadShaders(PipelineHandle h);
		static ShaderResourceProxy LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName);

		static void SetDebugName(BufferHandle h, const std::string& name);
		static void SetDebugName(TextureHandle h, const std::string& name);

		static const uint8* GetBufferData(BufferHandle h);

		static bool GetIsReady(BufferHandle h);
		static bool GetIsReady(TextureHandle h);

		static TexFormat GetTextureFormat(TextureHandle h);

		// Resource Transitions
		static void AddBarrier(BufferHandle h, ResourceState newState);
		static void AddBarrier(TextureHandle h, ResourceState newState);
		static void FlushBarriers();

		// Resource View Binding
		static void BindVertexBuffer(BufferHandle h, uint32 offset = 0, uint32 stride = 0);
		static void BindIndexBuffer(BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT);
		static void BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset = 0);

		static void SetPushConstants(const void* data, const uint32 size);

		static void BindSRV(BufferHandle h);
		static void BindSRV(TextureHandle h);
		static void BindUAV(TextureHandle h, uint32 mipLevel);

		// Query Bindless View Indices
		static uint32 GetBindlessSRV(BufferHandle h);
		static uint32 GetBindlessSRV(TextureHandle h);
		static uint32 GetBindlessUAV(TextureHandle h, uint32 mipLevel = 0);

		// - Pipeline State -------------------------------------------------------------------- //

		static void SetScissorRect(int4 rect);
		static void SetBlendFactor(float4 blend);

		// - Draw/Dispatch Calls --------------------------------------------------------------- //

		static void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0);
		static void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation);
		static void DrawFullscreenTriangle();

		static void Dispatch(uint3 threadGroupCount);

		// - Swap Chain/Back Buffers ----------------------------------------------------------- //

		static void ResizeSwapChainAndBackBuffers(uint2 newSize);

		static uint2 GetBackBufferSize();
		static TexFormat GetBackBufferFormat();

		// - Timestamps ------------------------------------------------------------------------ //

		static void BeginTimestamp(uint32 idx);
		static void EndTimestamp(uint32 idx);
		static void CollectTimestamps(BufferHandle h, uint32 count);
		static double GetTimestampFrequency();
	};

}