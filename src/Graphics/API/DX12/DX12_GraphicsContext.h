#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12SwapChain;
	class DX12GraphicsCommandList;

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

		BufferHandle CreateBuffer(const BufferDesc& desc, void* initialData = nullptr, size_t dataSize = 0) override;
		TextureHandle CreateTexture(const TextureDesc& desc) override;

		uint32 GetBindlessHeapIndex(const BufferHandle& h) override;

	private:
		void BeginRenderPassInternal();

		void OnWindowResizeEvent(WindowResizeEvent& event);

		Ptr<DX12Device> m_Device;
		Ptr<DX12SwapChain> m_SwapChain;
		Ptr<DX12GraphicsCommandList> m_GraphicsCommandList;

 		Ptr<HandlePool<Texture, NUM_TEXTURES>> m_TextureHandles;
 		Vector<DX12Texture> m_Textures;

		Ptr<HandlePool<Buffer, NUM_BUFFERS>> m_BufferHandles;
		Vector<DX12Buffer> m_Buffers;

		DX12Texture* m_CurrentRT;

		uint32 m_FrameId;
	};
}