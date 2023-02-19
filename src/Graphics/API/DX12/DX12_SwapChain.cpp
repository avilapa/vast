#include "vastpch.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include "Core/EventTypes.h"

#include <dxgi1_6.h>
#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

namespace vast::gfx
{

	DX12SwapChain::DX12SwapChain(const uint2& swapChainSize, const Format& swapChainFormat, const Format& backBufferFormat,
		DX12Device& device, HWND windowHandle /*= ::GetActiveWindow()*/)
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

	DX12SwapChain::~DX12SwapChain()
	{
		VAST_PROFILE_FUNCTION();

		DestroyBackBuffers();

		VAST_INFO("[gfx] [dx12] Destroying swapchain.");
		DX12SafeRelease(m_SwapChain);
	}

	DX12Texture& DX12SwapChain::GetCurrentBackBuffer() const
	{
		return *m_BackBuffers[m_SwapChain->GetCurrentBackBufferIndex()];
	}

	void DX12SwapChain::Present()
	{
		VAST_PROFILE_FUNCTION();

		constexpr uint32 kSyncInterval = ENABLE_VSYNC ? 1 : 0;
		constexpr uint32 kPresentFlags = (ALLOW_TEARING && !ENABLE_VSYNC) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		m_SwapChain->Present(kSyncInterval, kPresentFlags);
	}

	void DX12SwapChain::OnWindowResizeEvent(WindowResizeEvent& event)
	{
		VAST_PROFILE_FUNCTION();

		if (event.m_WindowSize.x != m_SwapChainSize.x || event.m_WindowSize.y != m_SwapChainSize.y)
		{
			VAST_ASSERTF(m_SwapChainSize.x != 0 && m_SwapChainSize.y != 0, "Failed to resize swapchain. Invalid window size.");
			m_SwapChainSize = event.m_WindowSize;

			m_Device.WaitForIdle();

			DestroyBackBuffers();
			{
				VAST_PROFILE_SCOPE("GFX", "ResizeBuffers");
				DXGI_SWAP_CHAIN_DESC scDesc = {};
				DX12Check(m_SwapChain->GetDesc(&scDesc));
				DX12Check(m_SwapChain->ResizeBuffers(NUM_BACK_BUFFERS, m_SwapChainSize.x, m_SwapChainSize.y, scDesc.BufferDesc.Format, scDesc.Flags));
			}
			m_Device.m_FrameId = m_SwapChain->GetCurrentBackBufferIndex();
			CreateBackBuffers();
		}
	}

	void DX12SwapChain::CreateBackBuffers()
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

			m_Device.GetDevice()->CreateRenderTargetView(backBuffer, &rtvDesc, backBufferRTVHandle.cpuHandle);

			m_BackBuffers[i] = MakePtr<DX12Texture>();
			m_BackBuffers[i]->resource = backBuffer;
			m_BackBuffers[i]->state = D3D12_RESOURCE_STATE_PRESENT;
			m_BackBuffers[i]->rtv = backBufferRTVHandle;
		}
	}

	void DX12SwapChain::DestroyBackBuffers()
	{
		VAST_PROFILE_FUNCTION();
		VAST_INFO("[gfx] [dx12] Destroying backbuffers.");

		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			m_Device.m_RTVStagingDescriptorHeap->FreeDescriptor(m_BackBuffers[i]->rtv);
			DX12SafeRelease(m_BackBuffers[i]->resource);
			m_BackBuffers[i] = nullptr;
		}
	}

	bool DX12SwapChain::CheckTearingSupport()
	{
		BOOL allowTearing = FALSE;
		if (FAILED(m_Device.m_DXGIFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			allowTearing = FALSE;
		}
		return allowTearing == TRUE;
	}

}