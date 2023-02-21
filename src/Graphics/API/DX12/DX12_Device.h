#pragma once

#include "Graphics/API/DX12/DX12_Common.h"
#include "Graphics/API/DX12/DX12_Descriptors.h"

struct IDXGIFactory7;

namespace D3D12MA
{
	class Allocator;
}

namespace vast::gfx
{
	class DX12CommandList;
	class DX12CommandQueue;

	enum class QueueType // TODO: Are we OK with this here?
	{
		GRAPHICS = 0,
		// TODO: Compute, Upload
		COUNT = 1,
	};

	class DX12Device
	{
		friend class DX12SwapChain;
	public:
		DX12Device();
		~DX12Device();

		void CreateBuffer(const BufferDesc& desc, DX12Buffer* buf);
		void CreateTexture(const TextureDesc& desc, DX12Texture* tex);
		void CreateShader(const ShaderDesc& desc, DX12Shader* shader);

		void BeginFrame(uint32 frameId);
		void EndFrame();
		void SubmitCommandList(DX12CommandList& ctx);
		void SignalEndOfFrame(uint32 frameId, const QueueType& type); // TODO: Made this public when moving out SwapChain
		void WaitForIdle();

		ID3D12Device5* GetDevice() const;
		DX12RenderPassDescriptorHeap& GetSRVDescriptorHeap(uint32 frameId) const;

		void CopyDescriptorsSimple(uint32 numDesc, D3D12_CPU_DESCRIPTOR_HANDLE destDescRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE srcDescRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE descType);

	private:
		IDXGIFactory7* m_DXGIFactory;
		ID3D12Device5* m_Device;
		D3D12MA::Allocator* m_Allocator;

		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		Ptr<DX12StagingDescriptorHeap> m_RTVStagingDescriptorHeap;
		Ptr<DX12StagingDescriptorHeap> m_DSVStagingDescriptorHeap;
		Ptr<DX12StagingDescriptorHeap> m_SRVStagingDescriptorHeap; // Serves CBV, SRV and UAV
		Vector<uint32> m_FreeReservedDescriptorIndices;
		Array<Ptr<DX12RenderPassDescriptorHeap>, NUM_FRAMES_IN_FLIGHT> m_SRVRenderPassDescriptorHeaps;

		void CopySRVHandleToReservedTable(DX12DescriptorHandle srvHandle, uint32 heapIndex);
	};
}