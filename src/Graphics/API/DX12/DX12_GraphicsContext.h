#pragma once

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

		void BeginRenderPass() override;
		void BeginRenderPass(const TextureHandle& h) override;
		void EndRenderPass() override;

		TextureHandle CreateTexture(const TextureDesc& desc) override;
		BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, size_t dataSize = 0) override;
		ShaderHandle CreateShader(const ShaderDesc& desc) override;

		void DestroyTexture(const TextureHandle& h) override;
		void DestroyBuffer(const BufferHandle& h) override;
		void DestroyShader(const ShaderHandle& h) override;

		uint32 GetBindlessHeapIndex(const BufferHandle& h) override;

	private:
		void SubmitCommandList(DX12CommandList& ctx);
		void SignalEndOfFrame(const QueueType& type);
		void WaitForIdle();

		void BeginRenderPassInternal();

		void OnWindowResizeEvent(WindowResizeEvent& event);

		void ProcessDestructions(uint32 frameId);

		Ptr<DX12Device> m_Device;
		Ptr<DX12SwapChain> m_SwapChain;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;
		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		// TODO: These should ideally be holding the API agnostic types instead of the DX12 ones.
		Ptr<ResourceHandlePool<DX12Texture, Texture, NUM_TEXTURES>> m_Textures;
		Ptr<ResourceHandlePool<DX12Buffer, Buffer, NUM_BUFFERS>> m_Buffers;
		Ptr<ResourceHandlePool<DX12Shader, Shader, NUM_SHADERS>> m_Shaders;

		Array<Vector<TextureHandle>, NUM_FRAMES_IN_FLIGHT> m_TexturesMarkedForDestruction;
		Array<Vector<BufferHandle>, NUM_FRAMES_IN_FLIGHT> m_BuffersMarkedForDestruction;

		DX12Texture* m_CurrentRT;

		uint32 m_FrameId;
	};
}