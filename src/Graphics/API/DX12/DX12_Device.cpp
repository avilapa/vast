#include "vastpch.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_ShaderCompiler.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include "dx12/D3D12MemoryAllocator/include/D3D12MemAlloc.h"
#include "dx12/DirectXAgilitySDK/include/d3d12shader.h"
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
		, m_ShaderCompiler(nullptr)
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

		{
			VAST_INFO("[gfx] [dx12] Creating shader compiler.");
			m_ShaderCompiler = MakePtr<DX12ShaderCompiler>();
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

		m_ShaderCompiler = nullptr;
		DX12SafeRelease(m_Allocator);
		DX12SafeRelease(m_Device);
		DX12SafeRelease(m_DXGIFactory);
	}

	void DX12Device::CreateBuffer(const BufferDesc& desc, DX12Buffer* outBuf)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(outBuf);

		outBuf->stride = desc.stride;

		uint32 nelem = static_cast<uint32>(desc.stride > 0 ? desc.size / desc.stride : 1);
		bool isHostVisible = ((desc.accessFlags & BufferAccessFlags::HOST_WRITABLE) == BufferAccessFlags::HOST_WRITABLE);

		D3D12_RESOURCE_STATES rscState = isHostVisible ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
		outBuf->state = rscState;

		D3D12_RESOURCE_DESC rscDesc = TranslateToDX12(desc);

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = isHostVisible ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
		m_Allocator->CreateResource(&allocDesc, &rscDesc, rscState, nullptr, &outBuf->allocation, IID_PPV_ARGS(&outBuf->resource));
		outBuf->gpuAddress = outBuf->resource->GetGPUVirtualAddress();

		if ((desc.viewFlags & BufferViewFlags::CBV) == BufferViewFlags::CBV)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = outBuf->gpuAddress;
			cbvDesc.SizeInBytes = static_cast<uint32>(rscDesc.Width);

			outBuf->cbv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateConstantBufferView(&cbvDesc, outBuf->cbv.cpuHandle);
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

			outBuf->srv = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateShaderResourceView(outBuf->resource, &srvDesc, outBuf->srv.cpuHandle);

			outBuf->heapIdx = m_FreeReservedDescriptorIndices.back();
			m_FreeReservedDescriptorIndices.pop_back();

			CopySRVHandleToReservedTable(outBuf->srv, outBuf->heapIdx);
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

			outBuf->uav = m_SRVStagingDescriptorHeap->GetNewDescriptor();
			m_Device->CreateUnorderedAccessView(outBuf->resource, nullptr, &uavDesc, outBuf->uav.cpuHandle);
		}

		if (isHostVisible)
		{
			outBuf->resource->Map(0, nullptr, reinterpret_cast<void**>(&outBuf->data));
		}
	}

	void DX12Device::CreateTexture(const TextureDesc& desc, DX12Texture* outTex)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(outTex);

		(void)desc;
		(void)outTex;
	}

	void DX12Device::CreatePipeline(const PipelineDesc& desc, DX12Pipeline* outPipeline)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(outPipeline);
		// TODO: Review shader ownership
		// TODO: What if a pipeline doesn't define a vs/ps?
		outPipeline->vs = MakePtr<DX12Shader>();
		m_ShaderCompiler->Compile(desc.vs, outPipeline->vs.get());
		outPipeline->ps = MakePtr<DX12Shader>();
		m_ShaderCompiler->Compile(desc.ps, outPipeline->ps.get());

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc = {};
		ID3DBlob* rsBlob = m_ShaderCompiler->CreateRootSignatureFromReflection(outPipeline);
		DX12Check(m_Device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&psDesc.pRootSignature)));
		DX12SafeRelease(rsBlob);

		psDesc.VS.pShaderBytecode = outPipeline->vs->blob->GetBufferPointer();
		psDesc.VS.BytecodeLength  = outPipeline->vs->blob->GetBufferSize();
		psDesc.PS.pShaderBytecode = outPipeline->ps->blob->GetBufferPointer();
		psDesc.PS.BytecodeLength  = outPipeline->ps->blob->GetBufferSize();
		
		psDesc.BlendState.AlphaToCoverageEnable = false;
		psDesc.BlendState.IndependentBlendEnable = false;
		for (uint32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			D3D12_RENDER_TARGET_BLEND_DESC* rtBlendDesc = &psDesc.BlendState.RenderTarget[i];
			rtBlendDesc->BlendEnable			= false;
			rtBlendDesc->LogicOpEnable			= false;
			rtBlendDesc->SrcBlend				= D3D12_BLEND_SRC_ALPHA;
			rtBlendDesc->DestBlend				= D3D12_BLEND_INV_SRC_ALPHA;
			rtBlendDesc->BlendOp				= D3D12_BLEND_OP_ADD;
			rtBlendDesc->SrcBlendAlpha			= D3D12_BLEND_ONE;
			rtBlendDesc->DestBlendAlpha			= D3D12_BLEND_ONE;
			rtBlendDesc->BlendOpAlpha			= D3D12_BLEND_OP_ADD;
			rtBlendDesc->LogicOp				= D3D12_LOGIC_OP_NOOP;
			rtBlendDesc->RenderTargetWriteMask	= D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		psDesc.SampleMask = 0xFFFFFFFF;

		psDesc.RasterizerState.FillMode					= D3D12_FILL_MODE_SOLID;
		psDesc.RasterizerState.CullMode					= D3D12_CULL_MODE_NONE;
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

		psDesc.DepthStencilState.DepthEnable	= false;
		psDesc.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
		psDesc.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psDesc.DepthStencilState.StencilEnable	= false;
		psDesc.DepthStencilState.FrontFace		= stencilOpDesc;
		psDesc.DepthStencilState.BackFace		= stencilOpDesc;

		psDesc.InputLayout.pInputElementDescs = nullptr;
		psDesc.InputLayout.NumElements = 0;

		psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psDesc.NumRenderTargets = desc.rtCount;
		for (uint32 i = 0; i < psDesc.NumRenderTargets; ++i)
		{
			psDesc.RTVFormats[i] = TranslateToDX12(desc.rtFormats[i]);
		}
		psDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

		// TODO: Multisample support
		psDesc.SampleDesc.Count = 1;
		psDesc.SampleDesc.Quality = 0;

		psDesc.NodeMask = 0;

		ID3D12PipelineState* graphicsPipeline = nullptr;
		DX12Check(m_Device->CreateGraphicsPipelineState(&psDesc, IID_PPV_ARGS(&graphicsPipeline)));

		outPipeline->pipelineState = graphicsPipeline;
		outPipeline->rootSignature = psDesc.pRootSignature;
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
			m_FreeReservedDescriptorIndices.push_back(tex->heapIdx);
		}

		if (tex->uav.IsValid())
		{
			m_SRVStagingDescriptorHeap->FreeDescriptor(tex->uav);
		}

		DX12SafeRelease(tex->resource);
		DX12SafeRelease(tex->allocation);
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
			m_FreeReservedDescriptorIndices.push_back(buf->heapIdx);
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

	void DX12Device::DestroyPipeline(DX12Pipeline* pipeline)
	{
		VAST_ASSERTF(pipeline, "Attempted to destroy an empty pipeline.");
		auto destroyShader = [](Ptr<DX12Shader> shader)
		{
			if (shader)
			{
				DX12SafeRelease(shader->blob);
				DX12SafeRelease(shader->reflection);
				shader = nullptr;
			}
		};

		destroyShader(std::move(pipeline->vs));
		destroyShader(std::move(pipeline->ps));
		DX12SafeRelease(pipeline->rootSignature);
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