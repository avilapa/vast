#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12SwapChain;
	class DX12CommandList;
	class DX12QueryHeap;
	class DX12GraphicsCommandList;
	class DX12UploadCommandList;
	class DX12CommandQueue;
	struct DX12RenderPassData;

	enum class QueueType
	{
		GRAPHICS = 0,
		// TODO: Compute
		UPLOAD,
		COUNT,
	};

	class DX12GraphicsContext final : public GraphicsContext
	{
	public:
		DX12GraphicsContext(const GraphicsParams& params);
		~DX12GraphicsContext();

		void FlushGPU() override;

		void BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE) override;
		void BeginRenderPass(PipelineHandle h, RenderPassTargets targets) override;
		void EndRenderPass() override;

		void AddBarrier(BufferHandle h, ResourceState newState) override;
		void AddBarrier(TextureHandle h, ResourceState newState) override;
		void FlushBarriers() override;

		void BindVertexBuffer(BufferHandle h, uint32 offset = 0, uint32 stride = 0) override;
		void BindIndexBuffer(BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT) override;
		void BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset = 0) override;
		void BindSRV(ShaderResourceProxy proxy, BufferHandle h) override;
		void BindSRV(ShaderResourceProxy proxy, TextureHandle h) override;
		void BindUAV(ShaderResourceProxy proxy, TextureHandle h) override;
		void SetPushConstants(const void* data, const uint32 size) override;

		void SetScissorRect(int4 rect) override;
		void SetBlendFactor(float4 blend) override;

		void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0) override;
		void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0) override;
		void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0) override;
		void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation) override;
		void DrawFullscreenTriangle() override;

		ShaderResourceProxy LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName) override;

		uint2 GetBackBufferSize() const override;
		float GetBackBufferAspectRatio() const override;
		TexFormat GetBackBufferFormat() const override;
		TexFormat GetTextureFormat(TextureHandle h) override;

		uint32 GetBindlessSRV(BufferHandle h) override;
		uint32 GetBindlessSRV(TextureHandle h) override;
		uint32 GetBindlessUAV(TextureHandle h) override;

		bool GetIsReady(BufferHandle h) override;
		bool GetIsReady(TextureHandle h) override;

		const uint8* GetBufferData(BufferHandle h) override;

		void SetDebugName(BufferHandle h, const std::string& name);
		void SetDebugName(TextureHandle h, const std::string& name);

	private:
		void BeginFrame_Internal() override;
		void EndFrame_Internal() override;

		void CreateBuffer_Internal(BufferHandle h, const BufferDesc& desc) override;
		void UpdateBuffer_Internal(BufferHandle h, const void* srcMem, size_t srcSize) override;
		void DestroyBuffer_Internal(BufferHandle h) override;

		void CreateTexture_Internal(TextureHandle h, const TextureDesc& desc) override;
		void UpdateTexture_Internal(TextureHandle h, const void* srcMem) override;
		void DestroyTexture_Internal(TextureHandle h) override;

		void CreatePipeline_Internal(PipelineHandle h, const PipelineDesc& desc) override;
		void UpdatePipeline_Internal(PipelineHandle h) override;
		void DestroyPipeline_Internal(PipelineHandle h) override;

		void BeginTimestamp_Internal(uint32 idx) override;
		void EndTimestamp_Internal(uint32 idx) override;
		void CollectTimestamps_Internal(BufferHandle h, uint32 count) override;

		void SubmitCommandList(DX12CommandList& ctx);
		void SignalEndOfFrame(const QueueType type);
		void WaitForIdle();

		void BeginRenderPass_Internal(const DX12RenderPassData& rpd);

		void OnWindowResizeEvent(const WindowResizeEvent& event);

		void CopyToDescriptorTable(const DX12Descriptor& srcDesc);

		uint32 GetBindlessIndex(DX12Descriptor& d);

		Ptr<DX12Device> m_Device;
		Ptr<DX12SwapChain> m_SwapChain;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;
		Array<Ptr<DX12UploadCommandList>, NUM_FRAMES_IN_FLIGHT> m_UploadCommandLists;

		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		using RenderPassEndBarrier = std::pair<DX12Texture*, D3D12_RESOURCE_STATES>;
		Vector<RenderPassEndBarrier> m_RenderPassEndBarriers;

		Ptr<ResourceHandler<DX12Buffer, Buffer, NUM_BUFFERS>> m_Buffers;
		Ptr<ResourceHandler<DX12Texture, Texture, NUM_TEXTURES>> m_Textures;
		Ptr<ResourceHandler<DX12Pipeline, Pipeline, NUM_PIPELINES>> m_Pipelines;

		Ptr<DX12QueryHeap> m_QueryHeap;
	};
}