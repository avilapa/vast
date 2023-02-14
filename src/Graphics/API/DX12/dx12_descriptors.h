#pragma once

#include "Graphics/graphics_context.h"

#include "dx12/DirectXAgilitySDK/include/d3d12.h"

namespace vast::gfx
{

	// - Descriptor Handles ----------------------------------------------------------------------- //

	struct DX12DescriptorHandle
	{
		bool IsValid() const { return m_CPUHandle.ptr != 0; }
		bool IsReferencedByShader() const { return m_GPUHandle.ptr != 0; }

		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle = { 0 };
		uint32 m_HeapIndex = 0;
	};

	// - Descriptor Heaps ------------------------------------------------------------------------- //

	class DX12DescriptorHeap
	{
	public:
		DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 maxDescriptors, bool bIsShaderVisible);
		virtual ~DX12DescriptorHeap();

		ID3D12DescriptorHeap* GetHeap() const;
		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetHeapCPUStart() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetHeapGPUStart() const;
		uint32 GetMaxDescriptors() const;
		uint32 GetDescriptorSize() const;

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