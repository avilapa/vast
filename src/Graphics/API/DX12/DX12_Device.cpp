#include "vastpch.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include "Core/EventTypes.h"

#include "dx12/D3D12MemoryAllocator/include/D3D12MemAlloc.h"

#include <dxgi1_6.h>
#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

#include <numeric>

namespace vast::gfx
{

	DX12Device::DX12Device()
		: m_DXGIFactory(nullptr)
		, m_Device(nullptr)
		, m_CommandQueues({ nullptr })
		, m_FrameFenceValues({ {0} })
		, m_RTVStagingDescriptorHeap(nullptr)
		, m_DSVStagingDescriptorHeap(nullptr)
		, m_SRVStagingDescriptorHeap(nullptr)
		, m_FreeReservedDescriptorIndices({ 0 })
		, m_SRVRenderPassDescriptorHeaps({ nullptr })
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();
		VAST_INFO("[gfx] [dx12] Starting graphics device creation.");

#ifdef VAST_DEBUG
		{
			VAST_PROFILE_SCOPE("Device", "EnableDebugLayer");
			ID3D12Debug* debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				DX12SafeRelease(debugController);
				VAST_INFO("[gfx] [dx12] Debug layer enabled.");
			}
		}
#endif // VAST_DEBUG

		{
			VAST_PROFILE_SCOPE("Device", "CreateDXGIFactory");
			VAST_INFO("[gfx] [dx12] Creating DXGI factory.");
			UINT createFactoryFlags = 0;
#ifdef VAST_DEBUG
			createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // VAST_DEBUG
			DX12Check(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_DXGIFactory)));
		}

		IDXGIAdapter1* adapter = nullptr;
		{
			VAST_PROFILE_SCOPE("Device", "Query adapters");
			uint32 bestAdapterIndex = 0;
			size_t bestAdapterMemory = 0;

			for (uint32 i = 0; m_DXGIFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				DXGI_ADAPTER_DESC1 adapterDesc;
				DX12Check(adapter->GetDesc1(&adapterDesc));

				// Choose adapter with the highest GPU memory.
				if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
					&& SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr))
					&& adapterDesc.DedicatedVideoMemory > bestAdapterMemory)
				{
					bestAdapterIndex = i;
					bestAdapterMemory = adapterDesc.DedicatedVideoMemory;
				}

				DX12SafeRelease(adapter);
			}

			VAST_INFO("[gfx] [dx12] GPU adapter found with index {}.", bestAdapterIndex);
			VAST_ASSERTF(bestAdapterMemory != 0, "Failed to find an adapter.");
			m_DXGIFactory->EnumAdapters1(bestAdapterIndex, &adapter);
		}

		{
			VAST_PROFILE_SCOPE("Device", "D3D12CreateDevice");
			VAST_INFO("[gfx] [dx12] Creating device.");
			DX12Check(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));
		}

		{
			VAST_PROFILE_SCOPE("Device", "D3D12MA::CreateAllocator");
			VAST_INFO("[gfx] [dx12] Creating allocator.");
			D3D12MA::ALLOCATOR_DESC desc = {};
			desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
			desc.pDevice = m_Device;
			desc.pAdapter = adapter;
			D3D12MA::CreateAllocator(&desc, &m_Allocator);
		}

		DX12SafeRelease(adapter);

#ifdef VAST_DEBUG
		VAST_INFO("[gfx] [dx12] Enabling debug messages.");
		ID3D12InfoQueue* infoQueue;
		if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, TRUE);

			//D3D12_MESSAGE_CATEGORY Categories[] = {};
			D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			//D3D12_MESSAGE_ID DenyIds[] = {};
			D3D12_INFO_QUEUE_FILTER filter = {};
			//filter.DenyList.NumCategories = _countof(Categories);
			//filter.DenyList.pCategoryList = Categories;
			filter.DenyList.NumSeverities = _countof(Severities);
			filter.DenyList.pSeverityList = Severities;
			//filter.DenyList.NumIDs = _countof(DenyIds);
			//filter.DenyList.pIDList = DenyIds;

			DX12Check(infoQueue->PushStorageFilter(&filter));

			DX12SafeRelease(infoQueue);
		}
#endif // VAST_DEBUG

		VAST_INFO("[gfx] [dx12] Creating command queues.");
		m_CommandQueues[IDX(QueueType::GRAPHICS)] = MakePtr<DX12CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

		VAST_INFO("[gfx] [dx12] Creating descriptor heaps.");
		m_RTVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 
			NUM_RTV_STAGING_DESCRIPTORS);
		m_DSVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 
			NUM_DSV_STAGING_DESCRIPTORS);
		m_SRVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 
			NUM_SRV_STAGING_DESCRIPTORS);

		m_FreeReservedDescriptorIndices.resize(NUM_RESERVED_SRV_DESCRIPTORS);
		std::iota(m_FreeReservedDescriptorIndices.begin(), m_FreeReservedDescriptorIndices.end(), 0);

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_SRVRenderPassDescriptorHeaps[i] = MakePtr<DX12RenderPassDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				NUM_RESERVED_SRV_DESCRIPTORS, NUM_SRV_RENDER_PASS_USER_DESCRIPTORS);
		}
	}

	DX12Device::~DX12Device()
	{
		VAST_PROFILE_FUNCTION();

		WaitForIdle();
		VAST_INFO("[gfx] [dx12] Starting graphics device destruction.");

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i] = nullptr;
		}

		m_RTVStagingDescriptorHeap = nullptr;
		m_DSVStagingDescriptorHeap = nullptr;
		m_SRVStagingDescriptorHeap = nullptr;

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_SRVRenderPassDescriptorHeaps[i] = nullptr;
		}

		DX12SafeRelease(m_Device);
		DX12SafeRelease(m_DXGIFactory);
	}

	Ref<DX12Buffer> DX12Device::CreateBuffer(const BufferDesc& desc)
	{
		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);

		Ref<DX12Buffer> newBufferRsc = MakeRef<DX12Buffer>();
		newBufferRsc->stride = desc.stride;

		uint32 nelem = static_cast<uint32>(desc.stride > 0 ? desc.size / desc.stride : 1);
		bool isHostVisible = ((desc.accessFlags & BufferAccessFlags::HOST_WRITABLE) == BufferAccessFlags::HOST_WRITABLE);

		D3D12_RESOURCE_STATES rscState = isHostVisible ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
		newBufferRsc->state = rscState;

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = isHostVisible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
		m_Allocator->CreateResource(&allocDesc, &rscDesc, rscState, nullptr, &newBufferRsc->allocation, IID_PPV_ARGS(&newBufferRsc->resource));
		newBufferRsc->gpuAddress = newBufferRsc->resource->GetGPUVirtualAddress();

		if ((desc.viewFlags & BufferViewFlags::CBV) == BufferViewFlags::CBV)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = newBufferRsc->gpuAddress;
			cbvDesc.SizeInBytes = static_cast<uint32>(rscDesc.Width);

			newBufferRsc->cbv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateConstantBufferView(&cbvDesc, newBufferRsc->cbv.cpuHandle);
		}

		if ((desc.viewFlags & BufferViewFlags::SRV) == BufferViewFlags::SRV)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = desc.isRawAccess ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = static_cast<uint32>(desc.isRawAccess ? (desc.size / 4) : nelem);
			srvDesc.Buffer.StructureByteStride = desc.isRawAccess ? 0 : desc.stride;
			srvDesc.Buffer.Flags = desc.isRawAccess ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

			newBufferRsc->srv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateShaderResourceView(newBufferRsc->resource, &srvDesc, newBufferRsc->srv.cpuHandle);

			newBufferRsc->heapIdx = m_FreeReservedDescriptorIndices.back();
			m_FreeReservedDescriptorIndices.pop_back();

			CopySRVHandleToReservedTable(newBufferRsc->srv, newBufferRsc->heapIdx);
		}

		if ((desc.viewFlags & BufferViewFlags::UAV) == BufferViewFlags::UAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = desc.isRawAccess ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = static_cast<uint32>(desc.isRawAccess ? (desc.size / 4) : nelem);
			uavDesc.Buffer.StructureByteStride = desc.isRawAccess ? 0 : desc.stride;
			uavDesc.Buffer.Flags = desc.isRawAccess ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

			newBufferRsc->uav = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateUnorderedAccessView(newBufferRsc->resource, nullptr, &uavDesc, newBufferRsc->uav.cpuHandle);
		}

		if (isHostVisible)
		{
			newBufferRsc->resource->Map(0, nullptr, reinterpret_cast<void**>(&newBufferRsc->data));
		}

		return newBufferRsc;
	}

	void DX12Device::CopyDescriptorsSimple(uint32 numDesc, D3D12_CPU_DESCRIPTOR_HANDLE destDescRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE srcDescRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE descType)
	{
		m_Device->CopyDescriptorsSimple(numDesc, destDescRangeStart, srcDescRangeStart, descType);
	}

	void DX12Device::CopySRVHandleToReservedTable(DX12DescriptorHandle srvHandle, uint32 heapIndex)
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DX12DescriptorHandle targetDescriptor = m_SRVRenderPassDescriptorHeaps[i]->GetReservedDescriptor(heapIndex);

			CopyDescriptorsSimple(1, targetDescriptor.cpuHandle, srvHandle.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	void DX12Device::BeginFrame()
	{
		VAST_PROFILE_FUNCTION();

		m_FrameId = (m_FrameId + 1) % NUM_FRAMES_IN_FLIGHT;

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i]->WaitForFenceValue(m_FrameFenceValues[i][m_FrameId]);
		}

		// TODO
	}

	void DX12Device::SignalEndOfFrame(const QueueType& type)
	{
		m_FrameFenceValues[IDX(type)][m_FrameId] = m_CommandQueues[IDX(type)]->SignalFence();
	}

	void DX12Device::EndFrame()
	{
		// TODO
	}

	void DX12Device::SubmitCommandList(DX12CommandList& ctx)
	{
		switch (ctx.GetCommandType())
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
			m_CommandQueues[IDX(QueueType::GRAPHICS)]->ExecuteCommandList(ctx.GetCommandList());
			break;
		default:
			VAST_ASSERTF(0, "Unsupported context submit type.");
			break;
		}
	}

	void DX12Device::WaitForIdle()
	{
		VAST_PROFILE_FUNCTION();

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i]->WaitForIdle();
		}
	}

	ID3D12Device5* DX12Device::GetDevice() const
	{
		return m_Device;
	}

	DX12RenderPassDescriptorHeap& DX12Device::GetSRVDescriptorHeap(uint32 frameId) const
	{
		return *m_SRVRenderPassDescriptorHeaps[frameId];
	}

	uint32 DX12Device::GetFrameId() const
	{
		return m_FrameId;
	}

}