#pragma once

#include "Graphics/API/DX12/DX12_Common.h"
#include "Graphics/API/DX12/DX12_Descriptors.h"

struct IDXGIFactory7;

namespace vast::gfx
{
	class DX12CommandList;
	class DX12CommandQueue;
	class DX12SwapChain;

	enum class QueueType // TODO: Are we OK with this here?
	{
		GRAPHICS = 0,
		COUNT = 1,
	};

	class DX12Device
	{
		friend class DX12SwapChain;
	public:
		DX12Device(const uint2& swapChainSize, const Format& swapChainFormat, const Format& backBufferFormat);
		~DX12Device();

		void BeginFrame();
		void EndFrame();
		void SubmitCommandList(DX12CommandList& ctx);
		void Present();
		void WaitForIdle();

		ID3D12Device5* GetDevice() const;
		DX12RenderPassDescriptorHeap& GetSRVDescriptorHeap(uint32 frameId) const;
		DX12Texture& GetCurrentBackBuffer() const;
		uint32 GetFrameId() const;
// 		uint2 GetSwapChainSize() const;
// 		Format GetSwapChainFormat() const;
// 		Format GetBackBufferFormat() const;

	private:
		IDXGIFactory7* m_DXGIFactory;
		ID3D12Device5* m_Device;

		Ptr<DX12SwapChain> m_SwapChain;

		void SignalEndOfFrame(const QueueType& type);

		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		Ptr<DX12StagingDescriptorHeap> m_RTVStagingDescriptorHeap;
		Array<Ptr<DX12RenderPassDescriptorHeap>, NUM_FRAMES_IN_FLIGHT> m_SRVRenderPassDescriptorHeaps;

		uint32 m_FrameId;
	};
}