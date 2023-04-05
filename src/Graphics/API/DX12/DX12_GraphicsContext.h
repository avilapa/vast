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

		void SetRenderTarget(const TextureHandle h) override;

		void BeginRenderPass(const PipelineHandle h, ClearParams clear = ClearParams()) override;
		void EndRenderPass() override;

		void SetVertexBuffer(const BufferHandle h, uint32 offset = 0, uint32 stride = 0) override;
		void SetIndexBuffer(const BufferHandle h, uint32 offset = 0, Format format = Format::UNKNOWN) override;
		void SetShaderResource(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy) override;
		void SetShaderResource(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy) override;
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
		PipelineHandle CreatePipeline(const PipelineDesc& desc) override;

		void UpdateBuffer(const BufferHandle h, void* data, const size_t size) override;

		void DestroyBuffer(const BufferHandle h) override;
		void DestroyTexture(const TextureHandle h) override;
		void DestroyPipeline(const PipelineHandle h) override;

		BufferView AllocTempBufferView(uint32 size, uint32 alignment = 0) override;

		ShaderResourceProxy LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName) override;

		Format GetBackBufferFormat() const override;
		uint32 GetBindlessIndex(const BufferHandle h) override;
		uint32 GetBindlessIndex(const TextureHandle h) override;
		bool GetIsReady(const TextureHandle h) override;

	private:
		void SubmitCommandList(DX12CommandList& ctx);
		void SignalEndOfFrame(const QueueType type);
		void WaitForIdle();

		void OnWindowResizeEvent(const WindowResizeEvent& event);

		void SetBufferData(DX12Buffer* buf, void* srcMem, size_t srcSize);

		void UploadBufferData(DX12Buffer* buf, void* srcMem, size_t srcSize);
		void UploadTextureData(DX12Texture* tex, void* srcMem);

		void ProcessDestructions(uint32 frameId);

		Ptr<DX12Device> m_Device;
		Ptr<DX12SwapChain> m_SwapChain;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;
		Array<Ptr<DX12UploadCommandList>, NUM_FRAMES_IN_FLIGHT> m_UploadCommandLists;

		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		Ptr<ResourceHandlePool<Buffer, DX12Buffer, NUM_BUFFERS>> m_Buffers;
		Ptr<ResourceHandlePool<Texture, DX12Texture, NUM_TEXTURES>> m_Textures;
		Ptr<ResourceHandlePool<Pipeline, DX12Pipeline, NUM_PIPELINES>> m_Pipelines;

		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction;
		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction;
		Array<Vector<PipelineHandle>, NUM_FRAMES_IN_FLIGHT> m_PipelinesMarkedForDestruction;

		struct TempAllocator
		{
			BufferHandle buffer;
			uint32 size = 0;
			uint32 offset = 0;

			void Reset() { offset = 0; }
		};
		// TODO: This could be replaced by a single, big dynamic buffer?
		Array<TempAllocator, NUM_FRAMES_IN_FLIGHT> m_TempFrameAllocators;

		DX12Texture* m_CurrentRT;

		uint32 m_FrameId;
	};
}