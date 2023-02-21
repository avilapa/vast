#include "vastpch.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include "dx12/D3D12MemoryAllocator/include/D3D12MemAlloc.h"
#include "dx12/DirectXShaderCompiler/inc/dxcapi.h"

#include <dxgi1_6.h>
#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

namespace vast::gfx
{

	DX12Device::DX12Device()
		: m_DXGIFactory(nullptr)
		, m_Device(nullptr)
		, m_RTVStagingDescriptorHeap(nullptr)
		, m_DSVStagingDescriptorHeap(nullptr)
		, m_SRVStagingDescriptorHeap(nullptr)
		, m_FreeReservedDescriptorIndices({ 0 })
		, m_SRVRenderPassDescriptorHeaps({ nullptr })
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
			// TODO: If we can't find the DirectXAgilitySDK dll's we will crash here. Check earlier and give a proper message.
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

		VAST_INFO("[gfx] [dx12] Creating descriptor heaps.");
		m_RTVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 
			NUM_RTV_STAGING_DESCRIPTORS);
		m_DSVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 
			NUM_DSV_STAGING_DESCRIPTORS);
		m_SRVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 
			NUM_SRV_STAGING_DESCRIPTORS);

		m_FreeReservedDescriptorIndices.resize(NUM_RESERVED_SRV_DESCRIPTORS);
		for (uint32 i = 0; i < m_FreeReservedDescriptorIndices.size(); ++i)
		{
			m_FreeReservedDescriptorIndices[i] = i;
		}

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_SRVRenderPassDescriptorHeaps[i] = MakePtr<DX12RenderPassDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				NUM_RESERVED_SRV_DESCRIPTORS, NUM_SRV_RENDER_PASS_USER_DESCRIPTORS);
		}
	}

	DX12Device::~DX12Device()
	{
		VAST_PROFILE_FUNCTION();
		VAST_INFO("[gfx] [dx12] Starting graphics device destruction.");

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

	void DX12Device::CreateBuffer(const BufferDesc& desc, DX12Buffer* buf)
	{
		VAST_PROFILE_FUNCTION();

		VAST_ASSERT(buf);
		buf->stride = desc.stride;

		uint32 nelem = static_cast<uint32>(desc.stride > 0 ? desc.size / desc.stride : 1);
		bool isHostVisible = ((desc.accessFlags & BufferAccessFlags::HOST_WRITABLE) == BufferAccessFlags::HOST_WRITABLE);

		D3D12_RESOURCE_STATES rscState = isHostVisible ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
		buf->state = rscState;

		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = isHostVisible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
		m_Allocator->CreateResource(&allocDesc, &rscDesc, rscState, nullptr, &buf->allocation, IID_PPV_ARGS(&buf->resource));
		buf->gpuAddress = buf->resource->GetGPUVirtualAddress();

		if ((desc.viewFlags & BufferViewFlags::CBV) == BufferViewFlags::CBV)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = buf->gpuAddress;
			cbvDesc.SizeInBytes = static_cast<uint32>(rscDesc.Width);

			buf->cbv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateConstantBufferView(&cbvDesc, buf->cbv.cpuHandle);
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

			buf->srv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateShaderResourceView(buf->resource, &srvDesc, buf->srv.cpuHandle);

			buf->heapIdx = m_FreeReservedDescriptorIndices.back();
			m_FreeReservedDescriptorIndices.pop_back();

			CopySRVHandleToReservedTable(buf->srv, buf->heapIdx);
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

			buf->uav = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateUnorderedAccessView(buf->resource, nullptr, &uavDesc, buf->uav.cpuHandle);
		}

		if (isHostVisible)
		{
			buf->resource->Map(0, nullptr, reinterpret_cast<void**>(&buf->data));
		}
	}

	void DX12Device::CreateTexture(const TextureDesc& desc, DX12Texture* tex)
	{
		VAST_PROFILE_FUNCTION();

		(void)desc;
		(void)tex;
	}

	void DX12Device::CreateShader(const ShaderDesc& desc, DX12Shader* shader)
	{
		VAST_PROFILE_FUNCTION();

		IDxcUtils* dxcUtils = nullptr;
		IDxcCompiler3* dxcCompiler = nullptr;
		IDxcIncludeHandler* dxcIncludeHandler = nullptr;

 		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
 		dxcUtils->CreateDefaultIncludeHandler(&dxcIncludeHandler);
 
 		std::wstring sourcePath;
 		sourcePath.append(SHADER_SOURCE_PATH);
 		sourcePath.append(desc.shaderName);

 		IDxcBlobEncoding* sourceBlobEncoding = nullptr;
		DX12Check(dxcUtils->LoadFile(sourcePath.c_str(), nullptr, &sourceBlobEncoding));

		DxcBuffer sourceBuffer = {};
		sourceBuffer.Ptr = sourceBlobEncoding->GetBufferPointer();
		sourceBuffer.Size = sourceBlobEncoding->GetBufferSize();
		sourceBuffer.Encoding = DXC_CP_ACP;

		LPCWSTR target = nullptr;
		switch (desc.type)
		{
		case ShaderType::COMPUTE:
			target = L"cs_6_6";
			break;
		case ShaderType::VERTEX:
			target = L"vs_6_6";
			break;
		case ShaderType::PIXEL:
			target = L"ps_6_6";
			break;
		default:
			VAST_ASSERTF(0, "Shader type not supported.");
			break;
		}

		std::vector<LPCWSTR> arguments;
		arguments.reserve(8);
		arguments.push_back(desc.shaderName.c_str());
		arguments.push_back(L"-E");
		arguments.push_back(desc.entryPoint.c_str());
		arguments.push_back(L"-T");
		arguments.push_back(target);
		arguments.push_back(L"-I");
		arguments.push_back(SHADER_SOURCE_PATH);
		arguments.push_back(L"-Zi");
		arguments.push_back(L"-WX");
		arguments.push_back(L"-Qstrip_reflect");

		IDxcResult* compilationResults = nullptr;
		dxcCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32>(arguments.size()), dxcIncludeHandler, IID_PPV_ARGS(&compilationResults));

		IDxcBlobUtf8* errors = nullptr;
		HRESULT getCompilationResults = compilationResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

		if (FAILED(getCompilationResults))
		{
			VAST_ASSERTF(0, "Failed to get compilation result.");
		}

		if (errors != nullptr && errors->GetStringLength() != 0)
		{
			wprintf(L"Shader compilation error:\n%S\n", errors->GetStringPointer());
			VAST_ASSERTF(0, "Shader compilation error.");
		}

		HRESULT statusResult;
		compilationResults->GetStatus(&statusResult);
		if (FAILED(statusResult))
		{
			VAST_ASSERTF(0, "Shader compilation failed.");
		}

		std::wstring dxilPath;
		std::wstring pdbPath;

		dxilPath.append(SHADER_OUTPUT_PATH);
		dxilPath.append(desc.shaderName);
		dxilPath.erase(dxilPath.end() - 5, dxilPath.end());
		dxilPath.append(L".dxil");

		pdbPath = dxilPath;
		pdbPath.append(L".pdb");

		IDxcBlob* shaderBlob = nullptr;
		compilationResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		if (shaderBlob != nullptr)
		{
			FILE* fp = nullptr;

			_wfopen_s(&fp, dxilPath.c_str(), L"wb");
			// TODO: Create "compiled" folder if not found, will crash newly cloned builds from repo otherwise.
			VAST_ASSERTF(fp, "Cannot find specified path.");

			fwrite(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 1, fp);
			fclose(fp);
		}

		IDxcBlob* pdbBlob = nullptr;
		compilationResults->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), nullptr);
		{
			FILE* fp = nullptr;

			_wfopen_s(&fp, pdbPath.c_str(), L"wb");
			VAST_ASSERTF(fp, "Cannot find specified path.");

			fwrite(pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize(), 1, fp);
			fclose(fp);
		}

		DX12SafeRelease(pdbBlob);
		DX12SafeRelease(errors);
		DX12SafeRelease(compilationResults);
		DX12SafeRelease(dxcIncludeHandler);
		DX12SafeRelease(dxcCompiler);
		DX12SafeRelease(dxcUtils);

		shader->shaderBlob = shaderBlob;
	}

	ID3D12Device5* DX12Device::GetDevice() const
	{
		return m_Device;
	}

	IDXGIFactory7* DX12Device::GetDXGIFactory() const
	{
		return m_DXGIFactory;
	}

	DX12RenderPassDescriptorHeap& DX12Device::GetSRVDescriptorHeap(uint32 frameId) const
	{
		return *m_SRVRenderPassDescriptorHeaps[frameId];
	}

	DX12DescriptorHandle DX12Device::CreateBackBufferRTV(ID3D12Resource* backBuffer, DXGI_FORMAT format)
	{
		VAST_ASSERT(backBuffer);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		DX12DescriptorHandle rtv = m_RTVStagingDescriptorHeap->GetNewDescriptor();
		m_Device->CreateRenderTargetView(backBuffer, &rtvDesc, rtv.cpuHandle);
		return rtv;
	}

	void DX12Device::DestroyBackBufferRTV(const DX12DescriptorHandle& rtv)
	{
		m_RTVStagingDescriptorHeap->FreeDescriptor(rtv);
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
}