#include "vastpch.h"
#include "Graphics/API/DX12/DX12_Descriptors.h"

namespace vast
{

	DX12DescriptorHeap::DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 maxDescriptors, bool bIsShaderVisible)
		: m_HeapType(heapType)
		, m_DescriptorHeap(nullptr)
		, m_MaxDescriptors(maxDescriptors)
		, m_DescriptorSize(0)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = m_MaxDescriptors;
		desc.Type = m_HeapType;
		desc.Flags = bIsShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		DX12Check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));
#if VAST_DEBUG
		std::string heapTypes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { "CBV/SRV/UAV", "SAMPLER", "RTV", "DSV" };
		std::string shaderVisible = (bIsShaderVisible ? "true" : "false");
		std::string heapName = "Descriptor Heap (Type: " + heapTypes[m_HeapType] + ", Shader Visible: " + shaderVisible + ", Max Descriptors: " + std::to_string(maxDescriptors) + ")";
		m_DescriptorHeap->SetName(std::wstring(heapName.begin(), heapName.end()).c_str());
#endif // VAST_DEBUG

		m_HeapStart.cpuHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		if (bIsShaderVisible)
		{
			m_HeapStart.gpuHandle = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		}

		// The size of a single descriptor in a descriptor heap is vendor specific
		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(m_HeapType);
	}

	DX12DescriptorHeap::~DX12DescriptorHeap()
	{
		DX12SafeRelease(m_DescriptorHeap);
	}

	DX12StagingDescriptorHeap::DX12StagingDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 maxDescriptors)
		: DX12DescriptorHeap(device, heapType, maxDescriptors, false)
		, m_CurrentDescriptorIndex(0)
		, m_ActiveHandleCount(0)
	{
		m_FreeDescriptors.reserve(m_MaxDescriptors);
	}

	DX12StagingDescriptorHeap::~DX12StagingDescriptorHeap()
	{
		if (m_ActiveHandleCount != 0)
		{
			VAST_ASSERTF(0, "There were {} active handles when the descriptor heap was destroyed.", m_ActiveHandleCount);
		}
	}

	DX12Descriptor DX12StagingDescriptorHeap::GetNewDescriptor()
	{
		std::lock_guard<std::mutex> lockGuard(m_UsageMutex);

		uint32 newHandleID = 0;

		if (m_CurrentDescriptorIndex < m_MaxDescriptors)
		{
			newHandleID = m_CurrentDescriptorIndex;
			m_CurrentDescriptorIndex++;
		}
		else if (m_FreeDescriptors.size() > 0)
		{
			newHandleID = m_FreeDescriptors.back();
			m_FreeDescriptors.pop_back();
		}
		else
		{
			VAST_ASSERTF(0, "Failed to create new descriptor. Increase heap size.");
		}

		m_ActiveHandleCount++;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_HeapStart.cpuHandle;
		cpuHandle.ptr += static_cast<uint64>(newHandleID) * m_DescriptorSize;

		DX12Descriptor desc;
		desc.heapIdx = newHandleID;
		desc.cpuHandle = cpuHandle;

		return desc;
	}

	void DX12StagingDescriptorHeap::FreeDescriptor(DX12Descriptor desc)
	{
		std::lock_guard<std::mutex> lockGuard(m_UsageMutex);

		m_FreeDescriptors.push_back(desc.heapIdx);

		if (m_ActiveHandleCount == 0)
		{
			VAST_ASSERTF(0, "Failed to free descriptor. There should be none left.");
			return;
		}

		m_ActiveHandleCount--;
	}

	DX12RenderPassDescriptorHeap::DX12RenderPassDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 reservedCount, uint32 userCount)
		: DX12DescriptorHeap(device, heapType, reservedCount + userCount, true)
		, m_CurrentDescriptorIndex(reservedCount)
		, m_ReservedHandleCount(reservedCount)
	{
	}

	DX12RenderPassDescriptorHeap::~DX12RenderPassDescriptorHeap()
	{
	}

	void DX12RenderPassDescriptorHeap::Reset()
	{
		m_CurrentDescriptorIndex = m_ReservedHandleCount;
	}

	DX12Descriptor DX12RenderPassDescriptorHeap::GetUserDescriptorBlockStart(uint32 count)
	{
		uint32 newHandleID = 0;

		{
			std::lock_guard<std::mutex> lockGuard(m_UsageMutex);

			uint32 blockEnd = m_CurrentDescriptorIndex + count;

			if (blockEnd <= m_MaxDescriptors)
			{
				newHandleID = m_CurrentDescriptorIndex;
				m_CurrentDescriptorIndex = blockEnd;
			}
			else
			{
				VAST_ASSERTF(0, "Failed to create new descriptor. Heap size is full at {}.", m_MaxDescriptors);
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_HeapStart.cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_HeapStart.gpuHandle;
		cpuHandle.ptr += static_cast<uint64>(newHandleID) * m_DescriptorSize;
		gpuHandle.ptr += static_cast<uint64>(newHandleID) * m_DescriptorSize;

		DX12Descriptor desc;
		desc.heapIdx = newHandleID;
		desc.cpuHandle = cpuHandle;
		desc.gpuHandle = gpuHandle;

		return desc;
	}

	DX12Descriptor DX12RenderPassDescriptorHeap::GetReservedDescriptor(uint32 index)
	{
		VAST_ASSERT(index < m_ReservedHandleCount);

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_HeapStart.cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_HeapStart.gpuHandle;
		cpuHandle.ptr += static_cast<uint64>(index) * m_DescriptorSize;
		gpuHandle.ptr += static_cast<uint64>(index) * m_DescriptorSize;

		DX12Descriptor desc;
		desc.heapIdx = index;
		desc.cpuHandle = cpuHandle;
		desc.gpuHandle = gpuHandle;

		return desc;
	}

}