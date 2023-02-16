#pragma once

#include "Graphics/API/DX12/dx12_common.h"

#include <mutex>

namespace vast::gfx
{

	class DX12DescriptorHeap
	{
	public:
		DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 maxDescriptors, bool bIsShaderVisible);
		virtual ~DX12DescriptorHeap();

		ID3D12DescriptorHeap* GetHeap() const { return m_DescriptorHeap; }
		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_HeapType; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetHeapCPUStart() const { return m_HeapStart.m_CPUHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetHeapGPUStart() const { return m_HeapStart.m_GPUHandle; }
		uint32 GetMaxDescriptors() const { return m_MaxDescriptors; }
		uint32 GetDescriptorSize() const { return m_DescriptorSize; }

	protected:
		D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
		ID3D12DescriptorHeap* m_DescriptorHeap;
		DX12DescriptorHandle m_HeapStart;
		uint32 m_MaxDescriptors;
		uint32 m_DescriptorSize;
	};

	class DX12StagingDescriptorHeap final : public DX12DescriptorHeap
	{
	public:
		DX12StagingDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 maxDescriptors);
		~DX12StagingDescriptorHeap();

		DX12DescriptorHandle GetNewDescriptor();
		void FreeDescriptor(DX12DescriptorHandle desc);

	private:
		Vector<uint32> m_FreeDescriptors;
		uint32 m_CurrentDescriptorIndex;
		uint32 m_ActiveHandleCount;
		std::mutex m_UsageMutex;
	};

	class DX12RenderPassDescriptorHeap final : public DX12DescriptorHeap
	{
	public:
		DX12RenderPassDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 reservedCount, uint32 userCount);
		~DX12RenderPassDescriptorHeap();

		void Reset();
		DX12DescriptorHandle GetUserDescriptorBlockStart(uint32 count);
		DX12DescriptorHandle GetReservedDescriptor(uint32 index);

	private:
		uint32 m_CurrentDescriptorIndex;
		uint32 m_ReservedHandleCount;
		std::mutex m_UsageMutex;
	};
}