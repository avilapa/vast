#include "vastpch.h"
#include "Graphics/API/DX12/dx12_device.h"
#include "Graphics/API/DX12/dx12_command_list.h"

#include "Core/event_types.h"

#include <dxgi1_6.h>
#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

// TODO: Long relative path will make it hard to simply export an .exe, but it is a lot more comfortable for development.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "..\\..\\..\\..\\vendor\\dx12\\DirectXAgilitySDK\\bin\\x64\\"; }

namespace vast::gfx
{

	class DX12CommandQueue
	{
	public:
		DX12CommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT)
			: m_CommandType(commandType)
			, m_Queue(nullptr)
			, m_Fence(nullptr)
			, m_NextFenceValue(1)
			, m_LastCompletedFenceValue(0)
			, m_FenceEventHandle(0)
		{
			VAST_PROFILE_FUNCTION();

			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = m_CommandType;
			desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.NodeMask = 0; // Used for MGPU

			DX12Check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_Queue)));
			DX12Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

			m_Fence->Signal(m_LastCompletedFenceValue);

			m_FenceEventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			VAST_ASSERTF(m_FenceEventHandle != INVALID_HANDLE_VALUE, "Failed to create fence event.");
		}
		
		~DX12CommandQueue()
		{
			VAST_PROFILE_FUNCTION();

			CloseHandle(m_FenceEventHandle);

			DX12SafeRelease(m_Fence);
			DX12SafeRelease(m_Queue);
		}

		bool IsFenceComplete(uint64 fenceValue)
		{
			if (fenceValue > m_LastCompletedFenceValue)
			{
				PollCurrentFenceValue();
			}

			return fenceValue <= m_LastCompletedFenceValue;
		}

		void WaitForFenceValue(uint64 fenceValue)
		{
			if (IsFenceComplete(fenceValue))
			{
				return;
			}

			{
				VAST_PROFILE_FUNCTION();

				std::lock_guard<std::mutex> lockGuard(m_EventMutex);

				m_Fence->SetEventOnCompletion(fenceValue, m_FenceEventHandle);
				WaitForSingleObjectEx(m_FenceEventHandle, INFINITE, false);
				m_LastCompletedFenceValue = fenceValue;
			}
		}		
		
		void WaitForIdle()
		{
			WaitForFenceValue(m_NextFenceValue - 1); // TODO: How is this different to Flush?
		}

		void Flush()
		{
			WaitForFenceValue(SignalFence());
		}

		uint64 PollCurrentFenceValue()
		{
			m_LastCompletedFenceValue = (std::max)(m_LastCompletedFenceValue, m_Fence->GetCompletedValue());
			return m_LastCompletedFenceValue;
		}

		uint64 SignalFence()
		{
			VAST_PROFILE_FUNCTION();

			std::lock_guard<std::mutex> lockGuard(m_FenceMutex);

			DX12Check(m_Queue->Signal(m_Fence, m_NextFenceValue));

			return m_NextFenceValue++;
		}

		uint64 ExecuteCommandList(ID3D12CommandList* commandList)
		{
			VAST_PROFILE_FUNCTION();

			DX12Check(static_cast<ID3D12GraphicsCommandList*>(commandList)->Close());
			m_Queue->ExecuteCommandLists(1, &commandList);

			return SignalFence();
		}

		ID3D12CommandQueue* GetQueue() 
		{ 
			return m_Queue; 
		}

	private:
		D3D12_COMMAND_LIST_TYPE m_CommandType;
		ID3D12CommandQueue* m_Queue;
		ID3D12Fence* m_Fence;
		uint64 m_NextFenceValue;
		uint64 m_LastCompletedFenceValue;
		HANDLE m_FenceEventHandle;
		std::mutex m_FenceMutex;
		std::mutex m_EventMutex;
	};

	//

	class DX12SwapChain
	{
	public:
		DX12SwapChain(const uint2& swapChainSize, const Format& swapChainFormat, const Format& backBufferFormat,
			DX12Device& device, HWND windowHandle = ::GetActiveWindow())
			: m_SwapChain(nullptr)
			, m_SwapChainSize(swapChainSize)
			, m_SwapChainFormat(swapChainFormat)
			, m_BackBufferFormat(backBufferFormat)
			, m_Device(device)
		{
			VAST_PROFILE_FUNCTION();
			VAST_INFO("[gfx] [dx12] Creating swapchain.");

			VAST_ASSERTF(m_SwapChainSize.x != 0 && m_SwapChainSize.y != 0, "Failed to create swapchain. Invalid swapchain size.");

			DXGI_SWAP_CHAIN_DESC1 scDesc = {};
			ZeroMemory(&scDesc, sizeof(scDesc));
			scDesc.Width = m_SwapChainSize.x;
			scDesc.Height = m_SwapChainSize.y;
			scDesc.Format = TranslateToDX12(m_SwapChainFormat);
			scDesc.Stereo = false;
			scDesc.SampleDesc = { 1, 0 };
			scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scDesc.BufferCount = NUM_BACK_BUFFERS;
			scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			scDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
			scDesc.Scaling = DXGI_SCALING_NONE;
			scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

			IDXGISwapChain1* swapChain = nullptr;
			auto queue = m_Device.m_CommandQueues[IDX(QueueType::GRAPHICS)]->GetQueue();
			DX12Check(m_Device.m_DXGIFactory->CreateSwapChainForHwnd(queue, windowHandle, &scDesc, nullptr, nullptr, &swapChain));
			DX12Check(swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain)));
			DX12SafeRelease(swapChain);

			CreateBackBuffers();

			VAST_SUBSCRIBE_TO_EVENT_DATA(WindowResizeEvent, DX12SwapChain::OnWindowResizeEvent);
		}

		~DX12SwapChain()
		{
			VAST_PROFILE_FUNCTION();

			DestroyBackBuffers();

			VAST_INFO("[gfx] [dx12] Destroying swapchain.");
			DX12SafeRelease(m_SwapChain);
		}

		Texture& GetCurrentBackBuffer() const
		{
			return *m_BackBuffers[m_SwapChain->GetCurrentBackBufferIndex()];
		}

		uint2 GetSwapChainSize() const
		{
			return m_SwapChainSize;
		}

		Format GetSwapChainFormat() const
		{
			return m_SwapChainFormat;
		}

		Format GetBackBufferFormat() const
		{
			return m_BackBufferFormat;
		}

		void Present()
		{
			m_SwapChain->Present(0, 0); // TODO: Vsync/Tearing flags
		}

	private:
		void OnWindowResizeEvent(WindowResizeEvent& event)
		{
			VAST_PROFILE_FUNCTION();

			uint32 newX = event.m_WindowSize.x;
			uint32 newY = event.m_WindowSize.y;
			uint2 asd = uint2(0, 0);

			if (newX != m_SwapChainSize.x || newY != m_SwapChainSize.y)
			{
				VAST_ASSERTF(m_SwapChainSize.x != 0 && m_SwapChainSize.y != 0, "Failed to resize swapchain. Invalid window size.");
				m_SwapChainSize = uint2(newX, newY);

 				m_Device.WaitForIdle();
 
 				DestroyBackBuffers();
 
 				DXGI_SWAP_CHAIN_DESC scDesc = {};
 				DX12Check(m_SwapChain->GetDesc(&scDesc));
				DX12Check(m_SwapChain->ResizeBuffers(NUM_BACK_BUFFERS, newX, newY, scDesc.BufferDesc.Format, scDesc.Flags));
 
				m_Device.m_FrameId = m_SwapChain->GetCurrentBackBufferIndex();
 
 				CreateBackBuffers();
			}
		}

		void CreateBackBuffers()
		{
			VAST_PROFILE_FUNCTION();
			VAST_INFO("[gfx] [dx12] Creating backbuffers.");

			for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
			{
				ID3D12Resource* backBuffer = nullptr;
				DX12Check(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
#if VAST_DEBUG
				// TODO: Figure out what we're doing with names
				std::string backBufferName = std::string("Back Buffer ") + std::to_string(i);
				backBuffer->SetName(std::wstring(backBufferName.begin(), backBufferName.end()).c_str());
#endif // VAST_DEBUG

				DX12DescriptorHandle backBufferRTVHandle = m_Device.m_RTVStagingDescriptorHeap->GetNewDescriptor();

				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = TranslateToDX12(m_BackBufferFormat);
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;
				rtvDesc.Texture2D.PlaneSlice = 0;

				m_Device.GetDevice()->CreateRenderTargetView(backBuffer, &rtvDesc, backBufferRTVHandle.m_CPUHandle);

				m_BackBuffers[i] = MakePtr<Texture>();
				m_BackBuffers[i]->m_Desc = backBuffer->GetDesc();
				m_BackBuffers[i]->m_Resource = backBuffer;
				m_BackBuffers[i]->m_State = D3D12_RESOURCE_STATE_PRESENT;
				m_BackBuffers[i]->m_RTVDescriptor = backBufferRTVHandle;
			}
		}

		void DestroyBackBuffers()
		{
			VAST_PROFILE_FUNCTION();
			VAST_INFO("[gfx] [dx12] Destroying backbuffers.");

			for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
			{
				m_Device.m_RTVStagingDescriptorHeap->FreeDescriptor(m_BackBuffers[i]->m_RTVDescriptor);
				DX12SafeRelease(m_BackBuffers[i]->m_Resource);
				m_BackBuffers[i] = nullptr;
			}
		}

		bool CheckTearingSupport()
		{
			BOOL allowTearing = FALSE;
			if (FAILED(m_Device.m_DXGIFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
			return allowTearing == TRUE;
		}

		DX12Device& m_Device;

		IDXGISwapChain4* m_SwapChain;
		Array<Ptr<Texture>, NUM_BACK_BUFFERS> m_BackBuffers;

		vast::uint2 m_SwapChainSize;
		Format m_SwapChainFormat;
		Format m_BackBufferFormat;
	};

	//

	DX12Device::DX12Device(const uint2& swapChainSize, const Format& swapChainFormat, const Format& backBufferFormat)
		: m_DXGIFactory(nullptr)
		, m_Device(nullptr)
		, m_SwapChain(nullptr)
		, m_CommandQueues({ nullptr })
		, m_FrameFenceValues({ {0} })
		, m_RTVStagingDescriptorHeap(nullptr)
		, m_SRVRenderPassDescriptorHeaps({ nullptr })
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();
		VAST_INFO("[gfx] [dx12] Starting graphics device creation.");

#ifdef VAST_DEBUG
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			VAST_PROFILE_SCOPE("Device", "EnableDebugLayer");
			debugController->EnableDebugLayer();
			DX12SafeRelease(debugController);
			VAST_INFO("[gfx] [dx12] Debug layer enabled.");
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

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_SRVRenderPassDescriptorHeaps[i] = MakePtr<DX12RenderPassDescriptorHeap>(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				NUM_RESERVED_SRV_DESCRIPTORS, NUM_SRV_RENDER_PASS_USER_DESCRIPTORS);
		}

		m_SwapChain = MakePtr<DX12SwapChain>(swapChainSize, swapChainFormat, backBufferFormat, *this);
	}

	DX12Device::~DX12Device()
	{
		VAST_PROFILE_FUNCTION();

		WaitForIdle();
		VAST_INFO("[gfx] [dx12] Starting graphics device destruction.");

		m_SwapChain = nullptr;

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i] = nullptr;
		}

		m_RTVStagingDescriptorHeap = nullptr;

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_SRVRenderPassDescriptorHeaps[i] = nullptr;
		}

		DX12SafeRelease(m_Device);
		DX12SafeRelease(m_DXGIFactory);
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

	void DX12Device::Present()
	{
		VAST_PROFILE_FUNCTION();

		m_SwapChain->Present();
		SignalEndOfFrame(QueueType::GRAPHICS);
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

	Texture& DX12Device::GetCurrentBackBuffer() const
	{
		return m_SwapChain->GetCurrentBackBuffer();
	}

	uint32 DX12Device::GetFrameId() const
	{
		return m_FrameId;
	}

}