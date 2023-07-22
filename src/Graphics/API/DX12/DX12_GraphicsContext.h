#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12SwapChain;
	class DX12CommandList;
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

		void BeginFrame() override;
		void EndFrame() override;
		bool IsInFrame() const override;

		void FlushGPU() override;

		void BeginRenderPassToBackBuffer(const PipelineHandle h, LoadOp loadOp = LoadOp::LOAD, StoreOp storeOp = StoreOp::STORE) override;
		void BeginRenderPass(const PipelineHandle h, const RenderPassTargets targets) override;
		void EndRenderPass() override;
		bool IsInRenderPass() const override;

		void SetVertexBuffer(const BufferHandle h, uint32 offset = 0, uint32 stride = 0) override;
		void SetIndexBuffer(const BufferHandle h, uint32 offset = 0, IndexBufFormat format = IndexBufFormat::R16_UINT) override;
		void SetConstantBufferView(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) override;
		void SetShaderResourceView(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) override;
		void SetShaderResourceView(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy) override;
		void SetPushConstants(const void* data, const uint32 size) override;

		void SetScissorRect(int4 rect) override;
		void SetBlendFactor(float4 blend) override;

		void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0) override;
		void DrawIndexed(uint32 idxCount, uint32 startIdxLocation = 0, uint32 baseVtxLocation = 0) override;
		void DrawInstanced(uint32 vtxCountPerInst, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0) override;
		void DrawIndexedInstanced(uint32 idxCountPerInstance, uint32 instanceCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation) override;
		void DrawFullscreenTriangle() override;

		BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, const size_t dataSize = 0) override;
		TextureHandle CreateTexture(const TextureDesc& desc, void* initialData = nullptr) override;
		TextureHandle CreateTexture(const std::string& filePath, bool sRGB = true) override;
		PipelineHandle CreatePipeline(const PipelineDesc& desc) override;

		void UpdateBuffer(const BufferHandle h, void* data, const size_t size) override;
		void UpdatePipeline(const PipelineHandle h) override;

		void DestroyBuffer(const BufferHandle h) override;
		void DestroyTexture(const TextureHandle h) override;
		void DestroyPipeline(const PipelineHandle h) override;

		// TODO: We need to attach descriptors to the BufferViews if we want to use them as CBVs, SRVs...
		BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0) override;

		ShaderResourceProxy LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName) override;

		uint2 GetBackBufferSize() const override;
		float GetBackBufferAspectRatio() const override;
		TexFormat GetBackBufferFormat() const override;
		TexFormat GetTextureFormat(const TextureHandle h) override;

		uint32 GetBindlessIndex(const BufferHandle h) override;
		uint32 GetBindlessIndex(const TextureHandle h) override;

		bool GetIsReady(const BufferHandle h) override;
		bool GetIsReady(const TextureHandle h) override;

		void SetDebugName(BufferHandle h, const std::string& name);
		void SetDebugName(TextureHandle h, const std::string& name);

	private:
		void SubmitCommandList(DX12CommandList& ctx);
		void SignalEndOfFrame(const QueueType type);
		void WaitForIdle();

		void ProcessDestructions(uint32 frameId);

		bool m_bHasFrameBegun;
		bool m_bHasRenderPassBegun;
		bool m_bHasBackBufferBeenRenderedToThisFrame;

		DX12RenderPassData SetupCommonRenderPassBarrierTransitions(DX12Pipeline* pipeline, RenderPassTargets targets);
		DX12RenderPassData SetupBackBufferRenderPassBarrierTransitions(LoadOp loadOp, StoreOp storeOp);
		void BeginRenderPass_Internal(const DX12RenderPassData& rpd);
		void BeginRenderPassToBackBuffer_Internal(DX12Pipeline* pipeline, LoadOp loadOp, StoreOp storeOp);
		void ValidateRenderPassTargets(DX12Pipeline* pipeline, RenderPassTargets targets) const;

		void OnWindowResizeEvent(const WindowResizeEvent& event);

		void UpdateBuffer_Internal(DX12Buffer* buf, void* srcMem, size_t srcSize);
		void UpdateTexture_Internal(DX12Texture* tex, void* srcMem);
		void SetShaderResourceView_Internal(const DX12Descriptor& srv);

		Ptr<DX12Device> m_Device;
		Ptr<DX12SwapChain> m_SwapChain;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;
		Array<Ptr<DX12UploadCommandList>, NUM_FRAMES_IN_FLIGHT> m_UploadCommandLists;

		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		using RenderPassEndBarrier = std::pair<DX12Texture*, D3D12_RESOURCE_STATES>;
		Vector<RenderPassEndBarrier> m_RenderPassEndBarriers;

		Ptr<HandlePool<DX12Buffer,   Buffer,   NUM_BUFFERS>>   m_Buffers;
		Ptr<HandlePool<DX12Texture,  Texture,  NUM_TEXTURES>>  m_Textures;
		Ptr<HandlePool<DX12Pipeline, Pipeline, NUM_PIPELINES>> m_Pipelines;

		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction;
		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction;
		Array<Vector<PipelineHandle>, NUM_FRAMES_IN_FLIGHT> m_PipelinesMarkedForDestruction;

		uint32 m_FrameId;
		
		struct TempAllocator
		{
			BufferHandle buffer;
			uint32 size = 0;
			uint32 offset = 0;

			void Reset() { offset = 0; }
		};
		// TODO: This could be replaced by a single, big dynamic buffer?
		Array<TempAllocator, NUM_FRAMES_IN_FLIGHT> m_TempFrameAllocators;

		void CreateTempFrameAllocators();
		void DestroyTempFrameAllocators();
	};
}