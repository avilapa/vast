#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{
	// - Graphics Backend ---------------------------------------------------------------------- //

	void Init(const GraphicsParams& params);
	void Shutdown();

	void BeginFrame();
	void EndFrame();
	uint32 GetFrameId();

	void BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE);
	void BeginRenderPass(PipelineHandle h, RenderPassTargets targets);
	void EndRenderPass();

	void BindPipelineForCompute(PipelineHandle h);

	void WaitForIdle();

	// - GPU Resources ------------------------------------------------------------------------- //

	void CreateBuffer(BufferHandle h, const BufferDesc& desc);
	void CreateTexture(TextureHandle h, const TextureDesc& desc);
	void CreatePipeline(PipelineHandle h, const PipelineDesc& desc);
	void CreatePipeline(PipelineHandle h, const ShaderDesc& csDesc);

	void DestroyBuffer(BufferHandle h);
	void DestroyTexture(TextureHandle h);
	void DestroyPipeline(PipelineHandle h);

	void UpdateBuffer(BufferHandle h, const void* srcMem, size_t srcSize);
	void UpdateTexture(TextureHandle h, const void* srcMem);

	void ReloadShaders(PipelineHandle h);
	ShaderResourceProxy LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName);

	void SetDebugName(BufferHandle h, const std::string& name);
	void SetDebugName(TextureHandle h, const std::string& name);

	const uint8* GetBufferData(BufferHandle h);

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

	void SetPushConstants(const void* data, const uint32 size);

	void BindSRV(BufferHandle h);
	void BindSRV(TextureHandle h);
	void BindUAV(TextureHandle h, uint32 mipLevel);

	// Query Bindless View Indices
	uint32 GetBindlessSRV(BufferHandle h);
	uint32 GetBindlessSRV(TextureHandle h);
	uint32 GetBindlessUAV(TextureHandle h, uint32 mipLevel = 0);

	// - Pipeline State ------------------------------------------------------------------------ //

	void SetScissorRect(int4 rect);
	void SetBlendFactor(float4 blend);

	// - Draw/Dispatch Calls ------------------------------------------------------------------- //

	void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0);
	void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation);
	void DrawFullscreenTriangle();

	void Dispatch(uint3 threadGroupCount);

	// - Swap Chain/Back Buffers --------------------------------------------------------------- //

	void ResizeSwapChainAndBackBuffers(uint2 newSize);

	uint2 GetBackBufferSize();
	TexFormat GetBackBufferFormat();

	// - Timestamps ---------------------------------------------------------------------------- //

	void BeginTimestamp(uint32 idx);
	void EndTimestamp(uint32 idx);
	void CollectTimestamps(BufferHandle h, uint32 count);
	double GetTimestampFrequency();

}