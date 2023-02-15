#pragma once

#include "Graphics/API/DX12/dx12_common.h"
#include "Graphics/API/DX12/dx12_descriptors.h"

struct IDXGIFactory7;
struct IDXGISwapChain4;

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
	public:
		DX12Device(const uint2& swapChainSize, const Format& swapChainFormat);
		~DX12Device();

		void BeginFrame();
		void EndFrame();
		void SubmitCommandList(DX12CommandList& ctx);
		void Present();
		void WaitForIdle();

		ID3D12Device5* GetDevice() const;
		DX12RenderPassDescriptorHeap& GetSRVDescriptorHeap(uint32 frameId) const;
		Texture& GetCurrentBackBuffer() const;
		uint32 GetFrameId() const;
// 		uint2 GetSwapChainSize() const;
// 		Format GetSwapChainFormat() const;
// 		Format GetBackBufferFormat() const;

		uint32 m_FrameId; // TODO: This should be private
	private:
		IDXGIFactory7* m_DXGIFactory;
		ID3D12Device5* m_Device;

		Ptr<DX12SwapChain> m_SwapChain;

		void SignalEndOfFrame(const QueueType& type);

		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		Ptr<DX12StagingDescriptorHeap> m_RTVStagingDescriptorHeap;
		Array<Ptr<DX12RenderPassDescriptorHeap>, NUM_FRAMES_IN_FLIGHT> m_SRVRenderPassDescriptorHeaps;
	};
}