#include "vastpch.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_ShaderManager.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include "dx12/D3D12MemoryAllocator/include/D3D12MemAlloc.h"
#include "dx12/DirectXShaderCompiler/inc/dxcapi.h"

#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

namespace vast
{
	constexpr D3D_FEATURE_LEVEL GetMinFeatureLevel()
	{
		return D3D_FEATURE_LEVEL_11_0;
	}

	const D3D_FEATURE_LEVEL GetMaxFeatureLevel(IDXGIAdapter4* adapter)
	{
		VAST_ASSERT(adapter);
		constexpr D3D_FEATURE_LEVEL availableFeatureLevels[]
		{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_2,
		};

		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsInfo = {};
		featureLevelsInfo.NumFeatureLevels = NELEM(availableFeatureLevels);
		featureLevelsInfo.pFeatureLevelsRequested = availableFeatureLevels;

		ID3D12Device* device;
		DX12Check(D3D12CreateDevice(adapter, GetMinFeatureLevel(), IID_PPV_ARGS(&device)));
		DX12Check(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsInfo, sizeof(featureLevelsInfo)));
		DX12SafeRelease(device);

		return featureLevelsInfo.MaxSupportedFeatureLevel;
	}

	//

	DX12Device::DX12Device()
		: m_DXGIFactory(nullptr)
		, m_Device(nullptr)
		, m_Allocator(nullptr)
		, m_ShaderManager(nullptr)
		, m_RTVStagingDescriptorHeap(nullptr)
		, m_DSVStagingDescriptorHeap(nullptr)
		, m_CBVSRVUAVStagingDescriptorHeap(nullptr)
		, m_DescriptorIndexFreeList()
		, m_CBVSRVUAVRenderPassDescriptorHeaps({ nullptr })
		, m_SamplerRenderPassDescriptorHeap(nullptr)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		UINT dxgiFactoryFlags = 0;
#ifdef VAST_DEBUG
		ID3D12Debug3* debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
		{
			// Note: Usage of debug layer requires "Graphics Tools" optional feature.
			debugInterface->EnableDebugLayer();
			//debugInterface->SetEnableGPUBasedValidation(true);
			VAST_LOG_WARNING("[gfx] [dx12] Debug layer enabled.");
			DX12SafeRelease(debugInterface);
		}

		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		VAST_LOG_TRACE("[gfx] [dx12] Creating DXGI factory.");
		DX12Check(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_DXGIFactory)));

		// TODO: Pass GPUAdapterPreferenceCriteria down through GraphicsContext as an option.
		IDXGIAdapter4* adapter = SelectMainAdapter(GPUAdapterPreferenceCriteria::MAX_PERF);

		VAST_LOG_TRACE("[gfx] [dx12] Creating main GFX device.");
		DX12Check(D3D12CreateDevice(adapter, GetMaxFeatureLevel(adapter), IID_PPV_ARGS(&m_Device)));
		m_Device->SetName(L"Main GFX Device");

		VAST_PROFILE_TRACE_BEGIN("Create Memory Allocator");
		VAST_LOG_TRACE("[gfx] [dx12] Creating memory allocator.");
		D3D12MA::ALLOCATOR_DESC allocatorDesc =
		{
			.Flags = D3D12MA::ALLOCATOR_FLAG_NONE,
			.pDevice = m_Device,
			.PreferredBlockSize = 0,
			.pAllocationCallbacks = nullptr,
			.pAdapter = adapter,
		};
		D3D12MA::CreateAllocator(&allocatorDesc, &m_Allocator);
		VAST_PROFILE_TRACE_END("Create Memory Allocator");

		DX12SafeRelease(adapter);

#ifdef VAST_DEBUG
		ID3D12InfoQueue* infoQueue;
		if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			D3D12_MESSAGE_SEVERITY Severities[] 
			{ 
				D3D12_MESSAGE_SEVERITY_INFO 
			};

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumSeverities = _countof(Severities);
			filter.DenyList.pSeverityList = Severities;

			DX12Check(infoQueue->PushStorageFilter(&filter));

			DX12SafeRelease(infoQueue);
		}
#endif

		VAST_LOG_TRACE("[gfx] [dx12] Creating descriptor heaps.");
		m_RTVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_RTV_STAGING_DESCRIPTORS);
		m_DSVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, NUM_DSV_STAGING_DESCRIPTORS);
		m_CBVSRVUAVStagingDescriptorHeap = MakePtr<DX12StagingDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_SRV_STAGING_DESCRIPTORS);

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_CBVSRVUAVRenderPassDescriptorHeaps[i] = MakePtr<DX12RenderPassDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_RESERVED_DESCRIPTOR_INDICES, NUM_RENDER_PASS_USER_DESCRIPTORS);
		}

		m_SamplerRenderPassDescriptorHeap = MakePtr<DX12RenderPassDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 0, IDX(SamplerState::COUNT));
		
		VAST_LOG_TRACE("[gfx] [dx12] Creating shader manager.");
		m_ShaderManager = MakePtr<DX12ShaderManager>();

		VAST_LOG_TRACE("[gfx] [dx12] Creating default samplers.");
		CreateSamplers();
	}

	IDXGIAdapter4* DX12Device::SelectMainAdapter(GPUAdapterPreferenceCriteria pref)
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_LOG_TRACE("[gfx] [dx12] Selecting best GPU adapter (based on '{}' criteria).", g_GPUAdapterPreferenceCriteriaNames[IDX(pref)]);

		// Note: If GPUAdapterPreferenceCriteria::HIGH_VRAM is selected we have to find it manually,
		// DXGI_GPU_PREFERENCE is set to UNSPECIFIED, and instead of returning the first enumerated
		// adapter we'll loop trough all of them and return one at the end, which makes this function
		// more convoluted.
		const DXGI_GPU_PREFERENCE dxgiGpuPref = TranslateToDX12(pref);
		uint32 highestVramAdapterIdx = 0;
		size_t highestVram = 0;

		IDXGIAdapter4* adapter = nullptr;
		for (uint32 i = 0; m_DXGIFactory->EnumAdapterByGpuPreference(i, dxgiGpuPref, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 desc;
			DX12Check(adapter->GetDesc1(&desc));

			if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			{
				DX12SafeRelease(adapter);
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(adapter, GetMinFeatureLevel(), __uuidof(ID3D12Device), nullptr)))
			{
				if (pref == GPUAdapterPreferenceCriteria::MAX_VRAM)
				{
					if (desc.DedicatedVideoMemory > highestVram)
					{
						highestVramAdapterIdx = i;
						highestVram = desc.DedicatedVideoMemory;
					}
				}
				else
				{
					break;
				}
			}

			DX12SafeRelease(adapter);
		}

		if (pref == GPUAdapterPreferenceCriteria::MAX_VRAM)
		{
			VAST_ASSERTF(highestVram != 0, "Failed to find an adapter.");
			m_DXGIFactory->EnumAdapterByGpuPreference(highestVramAdapterIdx, dxgiGpuPref, IID_PPV_ARGS(&adapter));
		}

		// Note: If we are crashing here, it's likely the system was unable to load D3D12Core.dll.
		// Check the logs and verify that D3D12SDKPath is correct.
		VAST_ASSERTF(adapter, "Failed to find an adapter.");

		DXGI_ADAPTER_DESC1 desc;
		DX12Check(adapter->GetDesc1(&desc));

		std::string gpuName(128, 0);
		WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, &gpuName[0], 128, nullptr, nullptr);

		VAST_LOG_TRACE("[gfx] [dx12] Found: {} ({} GB)", gpuName, int(desc.DedicatedVideoMemory / 1e9));
		return adapter;
	}

	DX12Device::~DX12Device()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_LOG_TRACE("[gfx] [dx12] Starting graphics device destruction.");

		m_RTVStagingDescriptorHeap = nullptr;
		m_DSVStagingDescriptorHeap = nullptr;
		m_CBVSRVUAVStagingDescriptorHeap = nullptr;

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_CBVSRVUAVRenderPassDescriptorHeaps[i] = nullptr;
		}
		m_SamplerRenderPassDescriptorHeap = nullptr;

		m_ShaderManager = nullptr;

		DX12SafeRelease(m_Allocator);
		DX12SafeRelease(m_DXGIFactory);

#ifdef VAST_DEBUG
		ID3D12InfoQueue* infoQueue;
		if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			// Note: Avoid debugbreak on ReportLiveDeviceObjects warning.
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			DX12SafeRelease(infoQueue);
		}
		ID3D12DebugDevice2* debugDevice;
		DX12Check(m_Device->QueryInterface(IID_PPV_ARGS(&debugDevice)));
		DX12SafeRelease(m_Device);
		// Note: Only one live device object is expected, and that is the debugDevice itself.
		DX12Check(debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
		DX12SafeRelease(debugDevice);
#else
		DX12SafeRelease(m_Device);
#endif
	}

	void DX12Device::CreateSamplers()
	{
		VAST_PROFILE_TRACE_FUNCTION;

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

		VAST_ASSERT(m_Device && m_ShaderManager);
		for (uint32 i = 0; i < IDX(SamplerState::COUNT); ++i)
		{
			m_Device->CreateSampler(&samplerDescs[i], currentSamplerDescriptor);
			currentSamplerDescriptor.ptr += m_SamplerRenderPassDescriptorHeap->GetDescriptorSize();

			size_t bufSize = strlen(g_SamplerNames[i]);
			std::wstring samplerName(bufSize, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, g_SamplerNames[i], -1, &samplerName[0], static_cast<int>(bufSize));

			m_ShaderManager->AddGlobalShaderDefine(samplerName + L"=" + std::to_wstring(i));
		}
	}

	void DX12Device::CreateBuffer(const BufferDesc& desc, DX12Buffer& outBuf)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		switch (desc.usage)
		{
		case ResourceUsage::DEFAULT:
			outBuf.state = D3D12_RESOURCE_STATE_COMMON;
			allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			break;
		case ResourceUsage::UPLOAD:
			outBuf.state = D3D12_RESOURCE_STATE_GENERIC_READ;
			allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			break;
		case ResourceUsage::READBACK:
			outBuf.state = D3D12_RESOURCE_STATE_COPY_DEST;
			allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
			break;
		default: VAST_ASSERT(0);
			break;
		}

		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);

		bool hasCBV = (desc.viewFlags & BufViewFlags::CBV) == BufViewFlags::CBV;
		bool hasSRV = (desc.viewFlags & BufViewFlags::SRV) == BufViewFlags::SRV;
		bool hasUAV = (desc.viewFlags & BufViewFlags::UAV) == BufViewFlags::UAV;

		if (hasCBV)
		{
			rscDesc.Width = static_cast<UINT64>(AlignU32(static_cast<uint32>(rscDesc.Width), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
		}

		// TODO: Do we need dynamic * NUM_FRAMES_IN_FLIGHT buffers?
		//rscDesc.Width *= NUM_FRAMES_IN_FLIGHT;

		m_Allocator->CreateResource(&allocDesc, &rscDesc, outBuf.state, nullptr, &outBuf.allocation, IID_PPV_ARGS(&outBuf.resource));
		outBuf.gpuAddress = outBuf.resource->GetGPUVirtualAddress();

		if (hasCBV)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = outBuf.gpuAddress;
			cbvDesc.SizeInBytes = static_cast<uint32>(rscDesc.Width);

			outBuf.cbv = m_CBVSRVUAVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateConstantBufferView(&cbvDesc, outBuf.cbv.cpuHandle);
		}

		const uint32 nelem = static_cast<uint32>(desc.stride > 0 ? desc.size / desc.stride : 1);

		if (hasSRV)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = desc.isRawAccess ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = static_cast<uint32>(desc.isRawAccess ? (desc.size / sizeof(uint32)) : nelem);
			srvDesc.Buffer.StructureByteStride = desc.isRawAccess ? 0 : desc.stride;
			srvDesc.Buffer.Flags = desc.isRawAccess ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

			outBuf.srv = m_CBVSRVUAVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateShaderResourceView(outBuf.resource, &srvDesc, outBuf.srv.cpuHandle);

			outBuf.srv.bindlessIdx = m_DescriptorIndexFreeList.AllocIndex();

			// TODO: For dynamic bindless vertex/index buffers we need to be able to update the SRVs accordingly
			CopyDescriptorToReservedTable(outBuf.srv, outBuf.srv.bindlessIdx);
		}

		if (hasUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = desc.isRawAccess ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = static_cast<uint32>(desc.isRawAccess ? (desc.size / sizeof(uint32)) : nelem);
			uavDesc.Buffer.StructureByteStride = desc.isRawAccess ? 0 : desc.stride;
			uavDesc.Buffer.Flags = desc.isRawAccess ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

			outBuf.uav = m_CBVSRVUAVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateUnorderedAccessView(outBuf.resource, nullptr, &uavDesc, outBuf.uav.cpuHandle);
		}

		if (desc.usage == ResourceUsage::UPLOAD || desc.usage == ResourceUsage::READBACK)
		{
			outBuf.resource->Map(0, nullptr, reinterpret_cast<void**>(&outBuf.data));
		}

		outBuf.usage = desc.usage;
		outBuf.stride = desc.stride;
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

	void DX12Device::CreateTexture(const TextureDesc& desc, DX12Texture& outTex)
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERTF(desc.width > 0 && desc.height > 0 && desc.depthOrArraySize > 0, "Invalid texture size.");
		VAST_ASSERTF(desc.mipCount <= MipLevelCount(desc.width, desc.height, desc.depthOrArraySize), "Invalid mip count.");

		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);

		bool hasRTV = (desc.viewFlags & TexViewFlags::RTV) == TexViewFlags::RTV;
		bool hasDSV = (desc.viewFlags & TexViewFlags::DSV) == TexViewFlags::DSV;
		bool hasSRV = (desc.viewFlags & TexViewFlags::SRV) == TexViewFlags::SRV;
		bool hasUAV = (desc.viewFlags & TexViewFlags::UAV) == TexViewFlags::UAV;

		D3D12_RESOURCE_STATES rscState = D3D12_RESOURCE_STATE_COMMON;
		DXGI_FORMAT srvFormat = rscDesc.Format;

		outTex.clearValue.Format = rscDesc.Format;

		if (hasRTV)
		{
			rscDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			rscState = D3D12_RESOURCE_STATE_RENDER_TARGET;

			outTex.clearValue.Color[0] = desc.clear.color.x;
			outTex.clearValue.Color[1] = desc.clear.color.y;
			outTex.clearValue.Color[2] = desc.clear.color.z;
			outTex.clearValue.Color[3] = desc.clear.color.w;
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

			outTex.clearValue.DepthStencil.Depth = desc.clear.ds.depth;
			outTex.clearValue.DepthStencil.Stencil = desc.clear.ds.stencil;
		}

		if (hasUAV)
		{
			rscDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			rscState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		outTex.state = rscState;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		// TODO: For texture readback we need to treat the resource as a Buffer... or just use a Buffer.
		m_Allocator->CreateResource(&allocationDesc, &rscDesc, rscState, (!hasRTV && !hasDSV) ? nullptr : &outTex.clearValue, &outTex.allocation, IID_PPV_ARGS(&outTex.resource));

		// TODO: Should TextureDesc be more explicit in whether a texture is a cubemap or not?
		bool bIsCubemap = (desc.type == TexType::TEXTURE_2D) && (desc.depthOrArraySize == 6);

		if (hasSRV)
		{
			outTex.srv = m_CBVSRVUAVStagingDescriptorHeap->GetNewDescriptor();

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

				m_Device->CreateShaderResourceView(outTex.resource, &srvDesc, outTex.srv.cpuHandle);
			}
			else
			{
				if (bIsCubemap)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.TextureCube.MostDetailedMip = 0;
					srvDesc.TextureCube.MipLevels = desc.mipCount;
					srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
					m_Device->CreateShaderResourceView(outTex.resource, &srvDesc, outTex.srv.cpuHandle);
				}
				else
				{
					// Null descriptor inherits the resource format and dimension
					m_Device->CreateShaderResourceView(outTex.resource, nullptr, outTex.srv.cpuHandle);
				}
			}

			outTex.srv.bindlessIdx = m_DescriptorIndexFreeList.AllocIndex();

			CopyDescriptorToReservedTable(outTex.srv, outTex.srv.bindlessIdx);
		}

		if (hasRTV)
		{
			outTex.rtv = m_RTVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateRenderTargetView(outTex.resource, nullptr, outTex.rtv.cpuHandle);
		}

		if (hasDSV)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = TranslateToDX12(desc.format);
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Texture2D.MipSlice = 0;

			outTex.dsv = m_DSVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateDepthStencilView(outTex.resource, &dsvDesc, outTex.dsv.cpuHandle);
		}

		if (hasUAV) // TODO: DSV exclusion?
		{
			if (desc.mipCount > 1)
			{
				outTex.uav.reserve(desc.mipCount);
				for (uint32 i = 0; i < desc.mipCount; ++i)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
					uavDesc.Format = TranslateToDX12(desc.format);
					if (bIsCubemap)
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						uavDesc.Texture2DArray.MipSlice = i;
						uavDesc.Texture2DArray.FirstArraySlice = 0;
						uavDesc.Texture2DArray.ArraySize = desc.depthOrArraySize;
					}
					else
					{
						uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						uavDesc.Texture2D.MipSlice = i;
					}

					outTex.uav.push_back(m_CBVSRVUAVStagingDescriptorHeap->GetNewDescriptor());
					m_Device->CreateUnorderedAccessView(outTex.resource, nullptr, &uavDesc, outTex.uav[i].cpuHandle);

					outTex.uav[i].bindlessIdx = m_DescriptorIndexFreeList.AllocIndex();
					CopyDescriptorToReservedTable(outTex.uav[i], outTex.uav[i].bindlessIdx);
				}
			}
			else
			{
				outTex.uav.push_back(m_CBVSRVUAVStagingDescriptorHeap->GetNewDescriptor());
				m_Device->CreateUnorderedAccessView(outTex.resource, nullptr, nullptr, outTex.uav[0].cpuHandle);

				outTex.uav[0].bindlessIdx = m_DescriptorIndexFreeList.AllocIndex();
				CopyDescriptorToReservedTable(outTex.uav[0], outTex.uav[0].bindlessIdx);
			}
		}

		// TODO: For UAVs, could we do better to identify when the UAV is actually ready?
		outTex.isReady = (hasRTV || hasDSV || hasUAV);
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

	void DX12Device::CreateGraphicsPipeline(const PipelineDesc& desc, DX12Pipeline& outPipeline)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc = {};

		if (desc.vs.type != ShaderType::UNKNOWN)
		{
			VAST_ASSERT(desc.vs.type == ShaderType::VERTEX);
			outPipeline.vs = m_ShaderManager->LoadShader(desc.vs);
			psDesc.VS.pShaderBytecode = outPipeline.vs->blob->GetBufferPointer();
			psDesc.VS.BytecodeLength = outPipeline.vs->blob->GetBufferSize();

			// Retrieve Input Layout from reflection
			auto paramsDesc = m_ShaderManager->GetInputParametersFromReflection(outPipeline.vs->reflection);
			D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[paramsDesc.size()]{};
			for (uint32 i = 0; i < paramsDesc.size(); ++i)
			{
				D3D12_INPUT_ELEMENT_DESC& element = inputElementDescs[i];

				element.SemanticName = paramsDesc[i].SemanticName;
				element.SemanticIndex = paramsDesc[i].SemanticIndex;
				element.Format = ConvertToDXGIFormat(paramsDesc[i].ComponentType, paramsDesc[i].Mask);
				element.InputSlot = 0;
				element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				// TODO: There doesn't seem to be a good way to automate instanced data.
				element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				element.InstanceDataStepRate = (element.InputSlotClass == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA) ? 1 : 0;
			}
			psDesc.InputLayout.pInputElementDescs = inputElementDescs;
			psDesc.InputLayout.NumElements = static_cast<uint32>(paramsDesc.size());
		}

		if (desc.ps.type != ShaderType::UNKNOWN)
		{
			VAST_ASSERT(desc.ps.type == ShaderType::PIXEL);
			outPipeline.ps = m_ShaderManager->LoadShader(desc.ps);
			psDesc.PS.pShaderBytecode = outPipeline.ps->blob->GetBufferPointer();
			psDesc.PS.BytecodeLength = outPipeline.ps->blob->GetBufferSize();
		}

		// Create root signature from reflection
		ID3DBlob* rootSignatureBlob = m_ShaderManager->CreateRootSignatureFromReflection(outPipeline);
		DX12Check(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&psDesc.pRootSignature)));
		DX12SafeRelease(rootSignatureBlob);
		outPipeline.rootSignature = psDesc.pRootSignature;

		// Fill the rest of the pipeline state
		psDesc.BlendState.AlphaToCoverageEnable = false;
		psDesc.BlendState.IndependentBlendEnable = false;
		for (uint32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			D3D12_RENDER_TARGET_BLEND_DESC* rtBlendDesc = &psDesc.BlendState.RenderTarget[i];
			rtBlendDesc->BlendEnable = desc.blendStates[i].blendEnable;
			rtBlendDesc->LogicOpEnable = false;
			rtBlendDesc->SrcBlend = TranslateToDX12(desc.blendStates[i].srcBlend);
			rtBlendDesc->DestBlend = TranslateToDX12(desc.blendStates[i].dstBlend);
			rtBlendDesc->BlendOp = TranslateToDX12(desc.blendStates[i].blendOp);
			rtBlendDesc->SrcBlendAlpha = TranslateToDX12(desc.blendStates[i].srcBlendAlpha);
			rtBlendDesc->DestBlendAlpha = TranslateToDX12(desc.blendStates[i].dstBlendAlpha);
			rtBlendDesc->BlendOpAlpha = TranslateToDX12(desc.blendStates[i].blendOpAlpha);
			rtBlendDesc->LogicOp = D3D12_LOGIC_OP_NOOP;
			rtBlendDesc->RenderTargetWriteMask = TranslateToDX12(desc.blendStates[i].writeMask);
		}

		psDesc.SampleMask = 0xFFFFFFFF;

		psDesc.RasterizerState.FillMode = TranslateToDX12(desc.rasterizerState.fillMode);
		psDesc.RasterizerState.CullMode = TranslateToDX12(desc.rasterizerState.cullMode);
		psDesc.RasterizerState.FrontCounterClockwise = false;
		psDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psDesc.RasterizerState.DepthClipEnable = true;
		psDesc.RasterizerState.MultisampleEnable = false;
		psDesc.RasterizerState.AntialiasedLineEnable = false;
		psDesc.RasterizerState.ForcedSampleCount = 0;
		psDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_DEPTH_STENCILOP_DESC stencilOpDesc = {};
		stencilOpDesc.StencilFailOp = stencilOpDesc.StencilDepthFailOp = stencilOpDesc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		stencilOpDesc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

		psDesc.DepthStencilState.DepthEnable = desc.depthStencilState.depthEnable;
		psDesc.DepthStencilState.DepthWriteMask = desc.depthStencilState.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		psDesc.DepthStencilState.DepthFunc = TranslateToDX12(desc.depthStencilState.depthFunc);
		psDesc.DepthStencilState.StencilEnable = false;
		psDesc.DepthStencilState.FrontFace = stencilOpDesc;
		psDesc.DepthStencilState.BackFace = stencilOpDesc;

		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		for (psDesc.NumRenderTargets = 0; psDesc.NumRenderTargets < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++psDesc.NumRenderTargets)
		{
			uint32 i = psDesc.NumRenderTargets;
			if (desc.renderPassLayout.rtFormats[i] == TexFormat::UNKNOWN)
			{
				break;
			}
			psDesc.RTVFormats[i] = TranslateToDX12(desc.renderPassLayout.rtFormats[i]);
		}
		psDesc.DSVFormat = TranslateToDX12(desc.renderPassLayout.dsFormat);

		// TODO: Multisample support
		psDesc.SampleDesc.Count = 1;
		psDesc.SampleDesc.Quality = 0;

		psDesc.NodeMask = 0;

		{
			VAST_PROFILE_TRACE_SCOPE("CreateGraphicsPipelineState (DX12)$");
			DX12Check(m_Device->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&outPipeline.pipelineState)));
		}
		outPipeline.desc = psDesc;
	}

	void DX12Device::CreateComputePipeline(const ShaderDesc& csDesc, DX12Pipeline& outPipeline)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		D3D12_COMPUTE_PIPELINE_STATE_DESC psDesc = {};

		VAST_ASSERT(csDesc.type == ShaderType::COMPUTE);
		outPipeline.cs = m_ShaderManager->LoadShader(csDesc);
		psDesc.CS.pShaderBytecode = outPipeline.cs->blob->GetBufferPointer();
		psDesc.CS.BytecodeLength = outPipeline.cs->blob->GetBufferSize();

		ID3DBlob* rootSignatureBlob = m_ShaderManager->CreateRootSignatureFromReflection(outPipeline);
		DX12Check(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&psDesc.pRootSignature)));
		DX12SafeRelease(rootSignatureBlob);
		outPipeline.rootSignature = psDesc.pRootSignature;

		psDesc.NodeMask = 0;

		{
			VAST_PROFILE_TRACE_SCOPE("CreateComputePipelineState (DX12)$");
			DX12Check(m_Device->CreateComputePipelineState(&psDesc, IID_PPV_ARGS(&outPipeline.pipelineState)));
		}
	}

	void DX12Device::ReloadShaders(DX12Pipeline& pipeline)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		if (pipeline.IsCompute())
		{
			VAST_ASSERT(pipeline.cs != nullptr);

			if (!m_ShaderManager->ReloadShader(pipeline.cs))
			{
				return;
			}

			D3D12_COMPUTE_PIPELINE_STATE_DESC psDesc = {};
			psDesc.CS.pShaderBytecode = pipeline.cs->blob->GetBufferPointer();
			psDesc.CS.BytecodeLength = pipeline.cs->blob->GetBufferSize();

			psDesc.pRootSignature = pipeline.rootSignature;
			psDesc.NodeMask = 0;

			DX12SafeRelease(pipeline.pipelineState);
			DX12Check(m_Device->CreateComputePipelineState(&psDesc, IID_PPV_ARGS(&pipeline.pipelineState)));
		}
		else
		{
			if (pipeline.vs != nullptr)
			{
				if (!m_ShaderManager->ReloadShader(pipeline.vs))
				{
					return;
				}
				pipeline.desc.VS.pShaderBytecode = pipeline.vs->blob->GetBufferPointer();
				pipeline.desc.VS.BytecodeLength = pipeline.vs->blob->GetBufferSize();
			}

			if (pipeline.ps != nullptr)
			{
				if (!m_ShaderManager->ReloadShader(pipeline.ps))
				{
					return;
				}
				pipeline.desc.PS.pShaderBytecode = pipeline.ps->blob->GetBufferPointer();
				pipeline.desc.PS.BytecodeLength = pipeline.ps->blob->GetBufferSize();
			}

			DX12SafeRelease(pipeline.pipelineState);
			DX12Check(m_Device->CreateGraphicsPipelineState(&pipeline.desc, IID_PPV_ARGS(&pipeline.pipelineState)));
		}
	}

	void DX12Device::DestroyBuffer(DX12Buffer& buf)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		if (buf.cbv.IsValid())
		{
			m_CBVSRVUAVStagingDescriptorHeap->FreeDescriptor(buf.cbv);
		}

		if (buf.srv.IsValid())
		{
			m_DescriptorIndexFreeList.FreeIndex(buf.srv.bindlessIdx);
			m_CBVSRVUAVStagingDescriptorHeap->FreeDescriptor(buf.srv);
		}

		if (buf.uav.IsValid())
		{
			m_CBVSRVUAVStagingDescriptorHeap->FreeDescriptor(buf.uav);
		}

		if (buf.data != nullptr)
		{
			buf.resource->Unmap(0, nullptr);
		}

		DX12SafeRelease(buf.resource);
		DX12SafeRelease(buf.allocation);
	}

	void DX12Device::DestroyTexture(DX12Texture& tex)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		if (tex.rtv.IsValid())
		{
			m_RTVStagingDescriptorHeap->FreeDescriptor(tex.rtv);
		}

		if (tex.dsv.IsValid())
		{
			m_DSVStagingDescriptorHeap->FreeDescriptor(tex.dsv);
		}

		if (tex.srv.IsValid())
		{
			m_DescriptorIndexFreeList.FreeIndex(tex.srv.bindlessIdx);
			m_CBVSRVUAVStagingDescriptorHeap->FreeDescriptor(tex.srv);
		}

		for (auto& i : tex.uav)
		{
			if (i.IsValid())
			{
				m_DescriptorIndexFreeList.FreeIndex(i.bindlessIdx);
				m_CBVSRVUAVStagingDescriptorHeap->FreeDescriptor(i);
			}
		}

		DX12SafeRelease(tex.resource);
		DX12SafeRelease(tex.allocation);
	}
	
	void DX12Device::DestroyPipeline(DX12Pipeline& pipeline)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		pipeline.vs = nullptr;
		pipeline.ps = nullptr;
		pipeline.cs = nullptr;
		if (pipeline.desc.InputLayout.pInputElementDescs)
		{
			delete[] pipeline.desc.InputLayout.pInputElementDescs;
			pipeline.desc.InputLayout.pInputElementDescs = nullptr;
		}
		DX12SafeRelease(pipeline.rootSignature);
		DX12SafeRelease(pipeline.pipelineState);
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
		return *m_CBVSRVUAVRenderPassDescriptorHeaps[frameId];
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

	void DX12Device::CopyDescriptorsSimple(uint32 numDesc, D3D12_CPU_DESCRIPTOR_HANDLE destDescRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE srcDescRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE descType)
	{
		m_Device->CopyDescriptorsSimple(numDesc, destDescRangeStart, srcDescRangeStart, descType);
	}

	void DX12Device::CopyDescriptorToReservedTable(DX12Descriptor srcDesc, uint32 heapIndex)
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DX12Descriptor dstDesc = m_CBVSRVUAVRenderPassDescriptorHeaps[i]->GetReservedDescriptor(heapIndex);
			CopyDescriptorsSimple(1, dstDesc.cpuHandle, srcDesc.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
}