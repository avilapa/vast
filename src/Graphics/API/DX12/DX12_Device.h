#pragma once

#include "Graphics/API/DX12/DX12_Common.h"
#include "Graphics/API/DX12/DX12_Descriptors.h"

struct IDXGIFactory7;

namespace D3D12MA
{
	class Allocator;
}

namespace vast
{
	class DX12ShaderManager;

	class DX12Device
	{
	public:
		DX12Device();
		~DX12Device();

		ID3D12Device5* GetDevice() const { return m_Device; };

		void CreateBuffer(const BufferDesc& desc, DX12Buffer& outBuf);
		void CreateTexture(const TextureDesc& desc, DX12Texture& outTex);
		void CreateGraphicsPipeline(const PipelineDesc& desc, DX12Pipeline& outPipeline);
		void CreateComputePipeline(const ShaderDesc& desc, DX12Pipeline& outPipeline);

		void ReloadShaders(DX12Pipeline& pipeline);

		void DestroyBuffer(DX12Buffer& buf);
		void DestroyTexture(DX12Texture& tex);
		void DestroyPipeline(DX12Pipeline& pipeline);

		IDXGISwapChain1* CreateSwapChain(ID3D12CommandQueue* graphicsQueue, WindowHandle windowHandle, uint32 bufferCount, uint2 size, DXGI_FORMAT format);
		DX12Descriptor CreateBackBufferRTV(ID3D12Resource* backBuffer, DXGI_FORMAT format);

		DX12RenderPassDescriptorHeap& GetSRVDescriptorHeap(uint32 frameId) const { return *m_CBVSRVUAVRenderPassDescriptorHeaps[frameId]; }
		DX12RenderPassDescriptorHeap& GetSamplerDescriptorHeap() const { return *m_SamplerRenderPassDescriptorHeap; }

	private:
		IDXGIAdapter4* SelectMainAdapter(GPUAdapterPreferenceCriteria pref);

		void CopyDescriptorToReservedTable(DX12Descriptor srvHandle, uint32 heapIndex);
		void CreateSamplers();

	private:
		IDXGIFactory7* m_DXGIFactory;
		ID3D12Device5* m_Device;
		D3D12MA::Allocator* m_Allocator;
		Ptr<DX12ShaderManager> m_ShaderManager;

		Ptr<DX12StagingDescriptorHeap> m_RTVStagingDescriptorHeap;
		Ptr<DX12StagingDescriptorHeap> m_DSVStagingDescriptorHeap;
		Ptr<DX12StagingDescriptorHeap> m_CBVSRVUAVStagingDescriptorHeap;

		FreeList<NUM_RESERVED_DESCRIPTOR_INDICES> m_DescriptorIndexFreeList;
		Array<Ptr<DX12RenderPassDescriptorHeap>, NUM_FRAMES_IN_FLIGHT> m_CBVSRVUAVRenderPassDescriptorHeaps;
		Ptr<DX12RenderPassDescriptorHeap> m_SamplerRenderPassDescriptorHeap;
	};
}