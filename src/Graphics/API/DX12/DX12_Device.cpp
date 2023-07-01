#include "vastpch.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_ShaderManager.h"
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
		, m_Allocator(nullptr)
		, m_ShaderManager(nullptr)
		, m_RTVStagingDescriptorHeap(nullptr)
		, m_DSVStagingDescriptorHeap(nullptr)
		, m_SRVStagingDescriptorHeap(nullptr)
		, m_FreeReservedDescriptorIndices({ 0 })
		, m_SRVRenderPassDescriptorHeaps({ nullptr })
		, m_SamplerRenderPassDescriptorHeap(nullptr)
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

		{
			VAST_INFO("[gfx] [dx12] Creating shader compiler.");
			m_ShaderManager = MakePtr<DX12ShaderManager>();
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

		m_SamplerRenderPassDescriptorHeap = MakePtr<DX12RenderPassDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 0, IDX(SamplerState::COUNT));
		CreateSamplers();
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
		m_SamplerRenderPassDescriptorHeap = nullptr;

		m_ShaderManager = nullptr;
		DX12SafeRelease(m_Allocator);
		DX12SafeRelease(m_Device);
		DX12SafeRelease(m_DXGIFactory);
	}

	void DX12Device::CreateSamplers()
	{
		D3D12_SAMPLER_DESC samplerDescs[IDX(SamplerState::COUNT)]{};
		{
			D3D12_SAMPLER_DESC& desc = samplerDescs[IDX(SamplerState::LINEAR_WRAP)];

			desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.MipLODBias = 0.0f;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
			desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0;
			desc.MinLOD = 0;
			desc.MaxLOD = D3D12_FLOAT32_MAX;
		}
		{
			D3D12_SAMPLER_DESC& desc = samplerDescs[IDX(SamplerState::LINEAR_CLAMP)];

			desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.MipLODBias = 0.0f;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
			desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0;
			desc.MinLOD = 0;
			desc.MaxLOD = D3D12_FLOAT32_MAX;
		}
		{
			D3D12_SAMPLER_DESC& desc = samplerDescs[IDX(SamplerState::POINT_CLAMP)];

			desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.MipLODBias = 0.0f;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
			desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0;
			desc.MinLOD = 0;
			desc.MaxLOD = D3D12_FLOAT32_MAX;
		}

		DX12Descriptor samplerDescriptorBlock = m_SamplerRenderPassDescriptorHeap->GetUserDescriptorBlockStart(static_cast<uint32>(IDX(SamplerState::COUNT)));
		D3D12_CPU_DESCRIPTOR_HANDLE currentSamplerDescriptor = samplerDescriptorBlock.cpuHandle;

		for (uint32 i = 0; i < IDX(SamplerState::COUNT); ++i)
		{
			m_Device->CreateSampler(&samplerDescs[i], currentSamplerDescriptor);
			currentSamplerDescriptor.ptr += m_SamplerRenderPassDescriptorHeap->GetDescriptorSize();

			m_ShaderManager->AddGlobalShaderDefine(std::string(g_SamplerNames[i]), std::to_string(i));
		}
	}

	void DX12Device::CreateBuffer(const BufferDesc& desc, DX12Buffer* outBuf)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(outBuf);

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		if (desc.cpuAccess == BufCpuAccess::NONE)
		{
			outBuf->state = D3D12_RESOURCE_STATE_COPY_DEST;
			allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		}
		else
		{
			outBuf->state = D3D12_RESOURCE_STATE_GENERIC_READ;
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		}
		// TODO: BufferCpuAccess::READBACK

		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);
		// TODO: Aligned width only needed for CBV?
		rscDesc.Width = static_cast<UINT64>(AlignU32(static_cast<uint32>(rscDesc.Width), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
		if (desc.usage == ResourceUsage::DYNAMIC)
		{
			rscDesc.Width *= NUM_FRAMES_IN_FLIGHT; // TODO: This is not currently used!
		}
		m_Allocator->CreateResource(&allocDesc, &rscDesc, outBuf->state, nullptr, &outBuf->allocation, IID_PPV_ARGS(&outBuf->resource));
		outBuf->gpuAddress = outBuf->resource->GetGPUVirtualAddress();

		if ((desc.viewFlags & BufViewFlags::CBV) == BufViewFlags::CBV)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = outBuf->gpuAddress;
			cbvDesc.SizeInBytes = static_cast<uint32>(rscDesc.Width);

			outBuf->cbv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateConstantBufferView(&cbvDesc, outBuf->cbv.cpuHandle);
		}

		const uint32 nelem = static_cast<uint32>(desc.stride > 0 ? desc.size / desc.stride : 1);

		if ((desc.viewFlags & BufViewFlags::SRV) == BufViewFlags::SRV)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = desc.isRawAccess ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = static_cast<uint32>(desc.isRawAccess ? (desc.size / sizeof(uint32)) : nelem);
			srvDesc.Buffer.StructureByteStride = desc.isRawAccess ? 0 : desc.stride;
			srvDesc.Buffer.Flags = desc.isRawAccess ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

			outBuf->srv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateShaderResourceView(outBuf->resource, &srvDesc, outBuf->srv.cpuHandle);

			outBuf->descriptorHeapIdx = m_FreeReservedDescriptorIndices.back();
			m_FreeReservedDescriptorIndices.pop_back();

			// TODO: For dynamic bindless vertex/index buffers we need to be able to update the SRVs accordingly
			CopySRVHandleToReservedTable(outBuf->srv, outBuf->descriptorHeapIdx);
		}

		if ((desc.viewFlags & BufViewFlags::UAV) == BufViewFlags::UAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = desc.isRawAccess ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = static_cast<uint32>(desc.isRawAccess ? (desc.size / sizeof(uint32)) : nelem);
			uavDesc.Buffer.StructureByteStride = desc.isRawAccess ? 0 : desc.stride;
			uavDesc.Buffer.Flags = desc.isRawAccess ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

			outBuf->uav = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateUnorderedAccessView(outBuf->resource, nullptr, &uavDesc, outBuf->uav.cpuHandle);
		}

		if (desc.cpuAccess == BufCpuAccess::WRITE)
		{
			// TODO: Persistent mapping may require manual GPU sync. Mapping/Unmapping every time may yield better performance
			outBuf->resource->Map(0, nullptr, reinterpret_cast<void**>(&outBuf->data));
		}

		outBuf->usage = desc.usage;
		outBuf->stride = desc.stride;
	}

	static uint32 MipLevelCount(uint32 width, uint32 height, uint32 depth = 1)
	{
		uint32 mipCount = 0;
		uint64 size = (std::max)((std::max)(width, height), depth);
		while ((1ull << mipCount) <= size)
		{
			++mipCount;
		}

		if ((1ull << mipCount) < size)
		{
			++mipCount;
		}

		return mipCount;
	}

	void DX12Device::CreateTexture(const TextureDesc& desc, DX12Texture* outTex)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(outTex);
		VAST_ASSERTF(desc.width > 0 && desc.height > 0 && desc.depthOrArraySize > 0, "Invalid texture size.");
		VAST_ASSERTF(desc.mipCount <= MipLevelCount(desc.width, desc.height, desc.depthOrArraySize), "Invalid mip count.");

		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);

		bool hasRTV = (desc.viewFlags & TexViewFlags::RTV) == TexViewFlags::RTV;
		bool hasDSV = (desc.viewFlags & TexViewFlags::DSV) == TexViewFlags::DSV;
		bool hasSRV = (desc.viewFlags & TexViewFlags::SRV) == TexViewFlags::SRV;
		bool hasUAV = (desc.viewFlags & TexViewFlags::UAV) == TexViewFlags::UAV;

		D3D12_RESOURCE_STATES rscState = D3D12_RESOURCE_STATE_COPY_DEST;
		DXGI_FORMAT srvFormat = rscDesc.Format;

		outTex->clearValue.Format = rscDesc.Format;

		if (hasRTV)
		{
			rscDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			rscState = D3D12_RESOURCE_STATE_RENDER_TARGET;

			outTex->clearValue.Color[0] = desc.clear.color.x;
			outTex->clearValue.Color[1] = desc.clear.color.y;
			outTex->clearValue.Color[2] = desc.clear.color.z;
			outTex->clearValue.Color[3] = desc.clear.color.w;
		}

		if (hasDSV)
		{
			rscDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			rscState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

			switch (TranslateToDX12(desc.format))
			{
			case DXGI_FORMAT_D16_UNORM:
				rscDesc.Format = DXGI_FORMAT_R16_TYPELESS;
				srvFormat = DXGI_FORMAT_R16_UNORM;
				break;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				rscDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
				srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT:
				rscDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				srvFormat = DXGI_FORMAT_R32_FLOAT;
				break;
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				rscDesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
				srvFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				VAST_ASSERTF(0, "Unknown depth stencil format.");
				break;
			}

			outTex->clearValue.DepthStencil.Depth = 1.0f;
			// TODO: Clear Stencil
		}

		if (hasUAV)
		{
			rscDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			rscState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		outTex->state = rscState;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		m_Allocator->CreateResource(&allocationDesc, &rscDesc, rscState, (!hasRTV && !hasDSV) ? nullptr : &outTex->clearValue, &outTex->allocation, IID_PPV_ARGS(&outTex->resource));

		if (hasSRV)
		{
			outTex->srv = m_SRVStagingDescriptorHeap->GetNewDescriptor();

			if (hasDSV)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = srvFormat;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.PlaneSlice = 0;
				srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

				m_Device->CreateShaderResourceView(outTex->resource, &srvDesc, outTex->srv.cpuHandle);
			}
			else
			{
				if ((desc.type == TexType::TEXTURE_2D) && (desc.depthOrArraySize == 6))
				{
					// Cubemap
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.TextureCube.MostDetailedMip = 0;
					srvDesc.TextureCube.MipLevels = desc.mipCount;
					srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
					m_Device->CreateShaderResourceView(outTex->resource, &srvDesc, outTex->srv.cpuHandle);
				}
				else
				{
					// Null descriptor inherits the resource format and dimension
					m_Device->CreateShaderResourceView(outTex->resource, nullptr, outTex->srv.cpuHandle);
				}
			}

			outTex->descriptorHeapIdx = m_FreeReservedDescriptorIndices.back();
			m_FreeReservedDescriptorIndices.pop_back();

			CopySRVHandleToReservedTable(outTex->srv, outTex->descriptorHeapIdx);
		}

		if (hasRTV)
		{
			outTex->rtv = m_RTVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateRenderTargetView(outTex->resource, nullptr, outTex->rtv.cpuHandle);
		}

		if (hasDSV)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = TranslateToDX12(desc.format);
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Texture2D.MipSlice = 0;

			outTex->dsv = m_DSVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateDepthStencilView(outTex->resource, &dsvDesc, outTex->dsv.cpuHandle);
		}

		if (hasUAV)
		{
			outTex->uav = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateUnorderedAccessView(outTex->resource, nullptr, nullptr, outTex->uav.cpuHandle);
		}

		outTex->isReady = (hasRTV || hasDSV);
	}

	static DXGI_FORMAT ConvertToDXGIFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask)
	{
		auto componentTypeSwitch = [](const D3D_REGISTER_COMPONENT_TYPE& type, DXGI_FORMAT optFloat, DXGI_FORMAT optUint, DXGI_FORMAT optSint) -> DXGI_FORMAT
		{
			switch (type)
			{
			case D3D_REGISTER_COMPONENT_FLOAT32: return optFloat;
			case D3D_REGISTER_COMPONENT_UINT32:  return optUint;
			case D3D_REGISTER_COMPONENT_SINT32:	 return optSint;
			default: VAST_ASSERTF(0, "Unknown input parameter format."); return DXGI_FORMAT_UNKNOWN;
			}
		};

		switch (mask)
		{
		case 0x01: return componentTypeSwitch(type, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_SINT);
		case 0x03: return componentTypeSwitch(type, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_SINT);
		case 0x07: return componentTypeSwitch(type, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32_UINT, DXGI_FORMAT_R32G32B32_SINT);
		case 0x0F: return componentTypeSwitch(type, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_SINT);
		default: VAST_ASSERTF(0, "Unknown input parameter format."); return DXGI_FORMAT_UNKNOWN;
		}
	}

	void DX12Device::CreatePipeline(const PipelineDesc& desc, DX12Pipeline* outPipeline)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(outPipeline);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc = {};

		if (desc.vs.type != ShaderType::UNKNOWN)
		{
			outPipeline->vs = m_ShaderManager->LoadShader(desc.vs);
			psDesc.VS.pShaderBytecode = outPipeline->vs->blob->GetBufferPointer();
			psDesc.VS.BytecodeLength  = outPipeline->vs->blob->GetBufferSize();

			auto paramsDesc = m_ShaderManager->GetInputParametersFromReflection(outPipeline->vs->reflection);
			D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[paramsDesc.size()]{};

			for (uint32 i = 0; i < paramsDesc.size(); ++i)
			{
				D3D12_INPUT_ELEMENT_DESC& element = inputElementDescs[i];

				element.SemanticName = paramsDesc[i].SemanticName;
				element.SemanticIndex = paramsDesc[i].SemanticIndex;
				element.Format = ConvertToDXGIFormat(paramsDesc[i].ComponentType, paramsDesc[i].Mask);
				element.InputSlot = 0;
				element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				element.InstanceDataStepRate = (element.InputSlotClass == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA) ? 1 : 0;
			}

			psDesc.InputLayout.pInputElementDescs = inputElementDescs;
			psDesc.InputLayout.NumElements = static_cast<uint32>(paramsDesc.size());
		}

		std::string shaderName = "Unknown";
		if (desc.ps.type != ShaderType::UNKNOWN)
		{
			outPipeline->ps = m_ShaderManager->LoadShader(desc.ps);
			psDesc.PS.pShaderBytecode = outPipeline->ps->blob->GetBufferPointer();
			psDesc.PS.BytecodeLength  = outPipeline->ps->blob->GetBufferSize();
			shaderName = desc.ps.shaderName;
		}

		outPipeline->resourceProxyTable = MakePtr<ShaderResourceProxyTable>(shaderName);

		ID3DBlob* rootSignatureBlob = m_ShaderManager->CreateRootSignatureFromReflection(outPipeline);
		DX12Check(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&psDesc.pRootSignature)));
		DX12SafeRelease(rootSignatureBlob);

		psDesc.BlendState.AlphaToCoverageEnable = false;
		psDesc.BlendState.IndependentBlendEnable = false;
		for (uint32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			D3D12_RENDER_TARGET_BLEND_DESC* rtBlendDesc = &psDesc.BlendState.RenderTarget[i];
			rtBlendDesc->BlendEnable			= desc.renderPassLayout.renderTargets[i].bs.blendEnable;
			rtBlendDesc->LogicOpEnable			= false;
			rtBlendDesc->SrcBlend				= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.srcBlend);
			rtBlendDesc->DestBlend				= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.dstBlend);
			rtBlendDesc->BlendOp				= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.blendOp);
			rtBlendDesc->SrcBlendAlpha			= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.srcBlendAlpha);
			rtBlendDesc->DestBlendAlpha			= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.dstBlendAlpha);
			rtBlendDesc->BlendOpAlpha			= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.blendOpAlpha);
			rtBlendDesc->LogicOp				= D3D12_LOGIC_OP_NOOP;
			rtBlendDesc->RenderTargetWriteMask	= TranslateToDX12(desc.renderPassLayout.renderTargets[i].bs.writeMask);
		}

		psDesc.SampleMask = 0xFFFFFFFF;

		psDesc.RasterizerState.FillMode					= TranslateToDX12(desc.rasterizerState.fillMode);
		psDesc.RasterizerState.CullMode					= TranslateToDX12(desc.rasterizerState.cullMode);
		psDesc.RasterizerState.FrontCounterClockwise	= false;
		psDesc.RasterizerState.DepthBias				= D3D12_DEFAULT_DEPTH_BIAS;
		psDesc.RasterizerState.DepthBiasClamp			= D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psDesc.RasterizerState.SlopeScaledDepthBias		= D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psDesc.RasterizerState.DepthClipEnable			= true;
		psDesc.RasterizerState.MultisampleEnable		= false;
		psDesc.RasterizerState.AntialiasedLineEnable	= false;
		psDesc.RasterizerState.ForcedSampleCount		= 0;
		psDesc.RasterizerState.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_DEPTH_STENCILOP_DESC stencilOpDesc = {};
		stencilOpDesc.StencilFailOp = stencilOpDesc.StencilDepthFailOp = stencilOpDesc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		stencilOpDesc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		psDesc.DepthStencilState.DepthEnable	= desc.depthStencilState.depthEnable;
		psDesc.DepthStencilState.DepthWriteMask	= desc.depthStencilState.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		psDesc.DepthStencilState.DepthFunc		= TranslateToDX12(desc.depthStencilState.depthFunc);
		psDesc.DepthStencilState.StencilEnable	= false;
		psDesc.DepthStencilState.FrontFace		= stencilOpDesc;
		psDesc.DepthStencilState.BackFace		= stencilOpDesc;

		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		for (psDesc.NumRenderTargets = 0; psDesc.NumRenderTargets < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++psDesc.NumRenderTargets)
		{
			uint32 i = psDesc.NumRenderTargets;
			if (desc.renderPassLayout.renderTargets[i].format == TexFormat::UNKNOWN)
			{
				break;
			}
			psDesc.RTVFormats[i] = TranslateToDX12(desc.renderPassLayout.renderTargets[i].format);
		}
		psDesc.DSVFormat = TranslateToDX12(desc.renderPassLayout.depthStencilTarget.format);

		// TODO: Multisample support
		psDesc.SampleDesc.Count = 1;
		psDesc.SampleDesc.Quality = 0;

		psDesc.NodeMask = 0;

		outPipeline->desc = psDesc;
		outPipeline->renderPassLayout = desc.renderPassLayout;
		DX12Check(m_Device->CreateGraphicsPipelineState(&outPipeline->desc, IID_PPV_ARGS(&outPipeline->pipelineState)));
	}

	void DX12Device::UpdatePipeline(DX12Pipeline* pipeline)
	{
		if (pipeline->vs != nullptr)
		{
			if (!m_ShaderManager->ReloadShader(pipeline->vs))
			{
				return;
			}
			pipeline->desc.VS.pShaderBytecode = pipeline->vs->blob->GetBufferPointer();
			pipeline->desc.VS.BytecodeLength = pipeline->vs->blob->GetBufferSize();
		}

		if (pipeline->ps != nullptr)
		{
			if (!m_ShaderManager->ReloadShader(pipeline->ps))
			{
				return;
			}			
			pipeline->desc.PS.pShaderBytecode = pipeline->ps->blob->GetBufferPointer();
			pipeline->desc.PS.BytecodeLength = pipeline->ps->blob->GetBufferSize();
		}

		DX12SafeRelease(pipeline->pipelineState);
		DX12Check(m_Device->CreateGraphicsPipelineState(&pipeline->desc, IID_PPV_ARGS(&pipeline->pipelineState)));
	}

	void DX12Device::DestroyBuffer(DX12Buffer* buf)
	{
		VAST_ASSERTF(buf, "Attempted to destroy an empty buffer.");

		if (buf->cbv.IsValid())
		{
			m_SRVStagingDescriptorHeap->FreeDescriptor(buf->cbv);
		}

		if (buf->srv.IsValid())
		{
			m_SRVStagingDescriptorHeap->FreeDescriptor(buf->srv);
			m_FreeReservedDescriptorIndices.push_back(buf->descriptorHeapIdx);
		}

		if (buf->uav.IsValid())
		{
			m_SRVStagingDescriptorHeap->FreeDescriptor(buf->uav);
		}

		if (buf->data != nullptr)
		{
			buf->resource->Unmap(0, nullptr);
		}

		DX12SafeRelease(buf->resource);
		DX12SafeRelease(buf->allocation);
	}

	void DX12Device::DestroyTexture(DX12Texture* tex)
	{
		VAST_ASSERTF(tex, "Attempted to destroy an empty texture.");

		if (tex->rtv.IsValid())
		{
			m_RTVStagingDescriptorHeap->FreeDescriptor(tex->rtv);
		}

		if (tex->dsv.IsValid())
		{
			m_DSVStagingDescriptorHeap->FreeDescriptor(tex->dsv);
		}

		if (tex->srv.IsValid())
		{
			m_SRVStagingDescriptorHeap->FreeDescriptor(tex->srv);
			m_FreeReservedDescriptorIndices.push_back(tex->descriptorHeapIdx);
		}

		if (tex->uav.IsValid())
		{
			m_SRVStagingDescriptorHeap->FreeDescriptor(tex->uav);
		}

		DX12SafeRelease(tex->resource);
		DX12SafeRelease(tex->allocation);
	}
	
	void DX12Device::DestroyPipeline(DX12Pipeline* pipeline)
	{
		VAST_ASSERTF(pipeline, "Attempted to destroy an empty pipeline.");
		pipeline->vs = nullptr;
		pipeline->ps = nullptr;
		if (pipeline->desc.InputLayout.pInputElementDescs)
		{
			delete[] pipeline->desc.InputLayout.pInputElementDescs;
			pipeline->desc.InputLayout.pInputElementDescs = nullptr;
		}
		DX12SafeRelease(pipeline->desc.pRootSignature);
		DX12SafeRelease(pipeline->pipelineState);
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

	DX12RenderPassDescriptorHeap& DX12Device::GetSamplerDescriptorHeap() const
	{
		return *m_SamplerRenderPassDescriptorHeap;
	}

	DX12Descriptor DX12Device::CreateBackBufferRTV(ID3D12Resource* backBuffer, DXGI_FORMAT format)
	{
		VAST_ASSERT(backBuffer);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		DX12Descriptor rtv = m_RTVStagingDescriptorHeap->GetNewDescriptor();
		m_Device->CreateRenderTargetView(backBuffer, &rtvDesc, rtv.cpuHandle);
		return rtv;
	}

	void DX12Device::DestroyBackBufferRTV(const DX12Descriptor& rtv)
	{
		m_RTVStagingDescriptorHeap->FreeDescriptor(rtv);
	}

	void DX12Device::CopyDescriptorsSimple(uint32 numDesc, D3D12_CPU_DESCRIPTOR_HANDLE destDescRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE srcDescRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE descType)
	{
		m_Device->CopyDescriptorsSimple(numDesc, destDescRangeStart, srcDescRangeStart, descType);
	}

	void DX12Device::CopySRVHandleToReservedTable(DX12Descriptor srvHandle, uint32 heapIndex)
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DX12Descriptor targetDescriptor = m_SRVRenderPassDescriptorHeaps[i]->GetReservedDescriptor(heapIndex);

			CopyDescriptorsSimple(1, targetDescriptor.cpuHandle, srvHandle.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
}