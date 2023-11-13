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
	class DX12ShaderManager;

	class DX12Device
	{
	public:
		DX12Device();
		~DX12Device();

		void CreateBuffer(const BufferDesc& desc, DX12Buffer& outBuf);
		void CreateTexture(const TextureDesc& desc, DX12Texture& outTex);
		void CreateGraphicsPipeline(const PipelineDesc& desc, DX12Pipeline& outPipeline);
		void CreateComputePipeline(const ShaderDesc& csDesc, DX12Pipeline& outPipeline);

		void ReloadShaders(DX12Pipeline& pipeline);

		void DestroyBuffer(DX12Buffer& buf);
		void DestroyTexture(DX12Texture& tex);
		void DestroyPipeline(DX12Pipeline& pipeline);

		ID3D12Device5* GetDevice() const;
		IDXGIFactory7* GetDXGIFactory() const;
		DX12RenderPassDescriptorHeap& GetSRVDescriptorHeap(uint32 frameId) const;
		DX12RenderPassDescriptorHeap& GetSamplerDescriptorHeap() const;
	
		// For internal use of the DX12SwapChain
		DX12Descriptor CreateBackBufferRTV(ID3D12Resource* backBuffer, DXGI_FORMAT format);

		void CopyDescriptorsSimple(uint32 numDesc, D3D12_CPU_DESCRIPTOR_HANDLE destDescRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE srcDescRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE descType);

	private:
		void CopyDescriptorToReservedTable(DX12Descriptor srvHandle, uint32 heapIndex);
		void CreateSamplers();

		IDXGIFactory7* m_DXGIFactory;
		ID3D12Device5* m_Device;
		D3D12MA::Allocator* m_Allocator;
		Ptr<DX12ShaderManager> m_ShaderManager;

		Ptr<DX12StagingDescriptorHeap> m_RTVStagingDescriptorHeap;
		Ptr<DX12StagingDescriptorHeap> m_DSVStagingDescriptorHeap;
		Ptr<DX12StagingDescriptorHeap> m_SRVStagingDescriptorHeap; // Serves CBV, SRV and UAV

		Vector<uint32> m_FreeReservedDescriptorIndices;
		Array<Ptr<DX12RenderPassDescriptorHeap>, NUM_FRAMES_IN_FLIGHT> m_SRVRenderPassDescriptorHeaps;
		Ptr<DX12RenderPassDescriptorHeap> m_SamplerRenderPassDescriptorHeap;
	};
}