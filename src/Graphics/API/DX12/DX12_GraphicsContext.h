#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12SwapChain;
	class DX12CommandList;
	class DX12GraphicsCommandList;
	class DX12CommandQueue;

	enum class QueueType
	{
		GRAPHICS = 0,
		// TODO: Compute, Upload
		COUNT = 1,
	};

	class DX12GraphicsContext final : public GraphicsContext
	{
	public:
		DX12GraphicsContext(const GraphicsParams& params);
		~DX12GraphicsContext();

		void BeginFrame() override;
		void EndFrame() override;

		void SetRenderTarget(const TextureHandle& h) override;
		void SetShaderResource(const BufferHandle& h, const std::string& shaderResourceName) override;
		void BeginRenderPass(const PipelineHandle& h) override;
		void EndRenderPass() override;

		void Draw(const uint32 vtxCount, const uint32 vtxStartLocation = 0) override;

		TextureHandle CreateTexture(const TextureDesc& desc) override;
		BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, size_t dataSize = 0) override;
		ShaderHandle CreateShader(const ShaderDesc& desc) override;
		PipelineHandle CreatePipeline(const PipelineDesc& desc) override;

		void DestroyTexture(const TextureHandle& h) override;
		void DestroyBuffer(const BufferHandle& h) override;
		void DestroyShader(const ShaderHandle& h) override;
		void DestroyPipeline(const PipelineHandle& h) override;

		uint32 GetBindlessHeapIndex(const BufferHandle& h) override;

	private:
		void SubmitCommandList(DX12CommandList& ctx);
		void SignalEndOfFrame(const QueueType& type);
		void WaitForIdle();

		void OnWindowResizeEvent(WindowResizeEvent& event);

		void ProcessDestructions(uint32 frameId);

		Ptr<DX12Device> m_Device;
		Ptr<DX12SwapChain> m_SwapChain;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;
		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		Ptr<ResourceHandlePool<Texture, DX12Texture, NUM_TEXTURES>> m_Textures;
		Ptr<ResourceHandlePool<Buffer, DX12Buffer, NUM_BUFFERS>> m_Buffers;
		Ptr<ResourceHandlePool<Shader, DX12Shader, NUM_SHADERS>> m_Shaders;
		Ptr<ResourceHandlePool<Pipeline, DX12Pipeline, NUM_PIPELINES>> m_Pipelines;

		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction;
		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction;
		Array<Vector<PipelineHandle>, NUM_FRAMES_IN_FLIGHT> m_PipelinesMarkedForDestruction;

		DX12Texture* m_CurrentRT;

		uint32 m_FrameId;
	};
}