#include "vastpch.h"
#include "Graphics/API/DX12/dx12_device.h"

#include <dxgidebug.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "..\\..\\..\\..\\vendor\\dx12\\DirectXAgilitySDK\\bin\\x64\\"; }

namespace vast::gfx
{

	DX12Device::DX12Device(const uint2& swapChainSize, const Format& swapChainFormat)
		: m_DXGIFactory(nullptr)
		, m_Device(nullptr)
		, m_CommandQueues({ nullptr })
		, m_SwapChainSize(swapChainSize)
		, m_SwapChainFormat(swapChainFormat)
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::DX12Device");

		CreateDeviceResources();
		CreateSwapChainResources();
	}

	DX12Device::~DX12Device()
	{
		WaitForIdle();

		DestroySwapChainResources();
		DestroyDeviceResources();
	}

	void DX12Device::CreateDeviceResources()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::CreateDeviceResources");
		VAST_INFO("[gfx] [dx12] Creating device resources.");

#ifdef VAST_DEBUG
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			DX12SafeRelease(debugController);
			VAST_INFO("[gfx] [dx12] Debug layer enabled.");
		}
#endif // VAST_DEBUG

		// Find GPU adapter.
		VAST_INFO("[gfx] [dx12] Creating DXGI factory.");
		UINT createFactoryFlags = 0;
#ifdef VAST_DEBUG
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // VAST_DEBUG
		DX12Check(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&m_DXGIFactory)));

		// Query adapters
		IDXGIAdapter1* adapter = nullptr;
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

		VAST_INFO("[gfx] [dx12] Creating device.");
		DX12Check(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device)));

		// TODO: Initialize Memory Allocator

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

			//	 		D3D12_MESSAGE_CATEGORY Categories[] = {};
			D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			// 			D3D12_MESSAGE_ID DenyIds[] = {};
			D3D12_INFO_QUEUE_FILTER filter = {};
			// 	 		filter.DenyList.NumCategories = _countof(Categories);
			// 	 		filter.DenyList.pCategoryList = Categories;
			filter.DenyList.NumSeverities = _countof(Severities);
			filter.DenyList.pSeverityList = Severities;
			// 			filter.DenyList.NumIDs = _countof(DenyIds);
			// 			filter.DenyList.pIDList = DenyIds;

			DX12Check(infoQueue->PushStorageFilter(&filter));

			DX12SafeRelease(infoQueue);
		}
#endif // VAST_DEBUG

		VAST_INFO("[gfx] [dx12] Creating command queues.");
		m_CommandQueues[CMD_QUEUE_TYPE_GRAPHICS] = std::make_unique<DX12CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_CommandQueues[CMD_QUEUE_TYPE_COPY] = std::make_unique<DX12CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_COPY);
	}

	void DX12Device::DestroyDeviceResources()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::DestroyDeviceResources");
		VAST_INFO("[gfx] [dx12] Destroying device resources.");

		for (uint32 i = 0; i < CMD_QUEUE_TYPE_COUNT; ++i)
		{
			m_CommandQueues[i] = nullptr;
		}

		DX12SafeRelease(m_Device);
		DX12SafeRelease(m_DXGIFactory);
	}

	void DX12Device::CreateSwapChainResources()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::CreateSwapChainResources");
		VAST_INFO("[gfx] [dx12] Creating swapchain resources.");

		VAST_ASSERTF(m_SwapChainSize.x != 0 && m_SwapChainSize.y != 0, "Failed to create swapchain. Invalid swapchain size.");

		VAST_INFO("[gfx] [dx12] Creating swapchain.");
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		ZeroMemory(&scDesc, sizeof(scDesc));
		scDesc.Width = m_SwapChainSize.x;
		scDesc.Height = m_SwapChainSize.y;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: m_SwapChainFormat
		scDesc.Stereo = false;
		scDesc.SampleDesc = { 1, 0 };
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = NUM_BACK_BUFFERS;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		scDesc.Scaling = DXGI_SCALING_NONE;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		IDXGISwapChain1* swapChain = nullptr;
		DX12Check(m_DXGIFactory->CreateSwapChainForHwnd(m_CommandQueues[CMD_QUEUE_TYPE_GRAPHICS]->GetQueue(), ::GetActiveWindow(), &scDesc, nullptr, nullptr, &swapChain));
		DX12Check(swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain)));
		DX12SafeRelease(swapChain);

		CreateBackBuffers();
	}

	void DX12Device::DestroySwapChainResources()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::DestroySwapChainResources");
		VAST_INFO("[gfx] [dx12] Destroying swapchain resources.");

		DestroyBackBuffers();
 		DX12SafeRelease(m_SwapChain);
	}

	void DX12Device::CreateBackBuffers()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::CreateBackBuffers");
		VAST_INFO("[gfx] [dx12] Creating backbuffers.");

	}

	void DX12Device::DestroyBackBuffers()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12Device::DestroyBackBuffers");
		VAST_INFO("[gfx] [dx12] Destroying backbuffers.");

	}

	bool DX12Device::CheckTearingSupport()
	{
		BOOL allowTearing = FALSE;
		if (FAILED(m_DXGIFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			allowTearing = FALSE;
		}
		return allowTearing == TRUE;
	}

	void DX12Device::WaitForIdle()
	{
		for (uint32 i = 0; i < CMD_QUEUE_TYPE_GRAPHICS; ++i)
		{
			m_CommandQueues[i]->WaitForIdle();
		}
	}

}