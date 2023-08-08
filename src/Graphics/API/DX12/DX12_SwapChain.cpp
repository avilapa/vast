#include "vastpch.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include <dxgi1_6.h>
#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

namespace vast::gfx
{
	static_assert(NUM_BACK_BUFFERS <= DXGI_MAX_SWAP_CHAIN_BUFFERS);

	DX12SwapChain::DX12SwapChain(const uint2& size, const TexFormat& format, const TexFormat& backBufferFormat,
		DX12Device& device, ID3D12CommandQueue& graphicsQueue, HWND windowHandle /*= ::GetActiveWindow()*/)
		: m_SwapChain(nullptr)
		, m_Size(size)
		, m_Format(format)
		, m_BackBufferFormat(backBufferFormat)
		, m_Device(device)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create SwapChain");
		VAST_LOG_TRACE("[gfx] [dx12] Creating swapchain.");

		VAST_ASSERTF(m_Size.x != 0 && m_Size.y != 0, "Failed to create swapchain. Invalid swapchain size.");

		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		ZeroMemory(&scDesc, sizeof(scDesc));
		scDesc.Width = m_Size.x;
		scDesc.Height = m_Size.y;
		scDesc.Format = TranslateToDX12(m_Format);
		scDesc.Stereo = false;
		scDesc.SampleDesc = { 1, 0 };
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = NUM_BACK_BUFFERS;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		scDesc.Scaling = DXGI_SCALING_NONE;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		IDXGISwapChain1* swapChain = nullptr;
		DX12Check(m_Device.GetDXGIFactory()->CreateSwapChainForHwnd(&graphicsQueue, windowHandle, &scDesc, nullptr, nullptr, &swapChain));
		DX12Check(swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain)));
		DX12SafeRelease(swapChain);

		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			m_BackBuffers[i] = MakePtr<DX12Texture>();
		}
		CreateBackBuffers();
	}

	DX12SwapChain::~DX12SwapChain()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy SwapChain");
		DestroyBackBuffers();
		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			m_BackBuffers[i] = nullptr;
		}

		VAST_LOG_TRACE("[gfx] [dx12] Destroying swapchain.");
		DX12SafeRelease(m_SwapChain);
	}

	DX12Texture& DX12SwapChain::GetCurrentBackBuffer() const
	{
		return *m_BackBuffers[m_SwapChain->GetCurrentBackBufferIndex()];
	}

	void DX12SwapChain::Present()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Present");

		constexpr uint32 kSyncInterval = ENABLE_VSYNC ? 1 : 0;
		constexpr uint32 kPresentFlags = (ALLOW_TEARING && !ENABLE_VSYNC) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		m_SwapChain->Present(kSyncInterval, kPresentFlags);
	}

	uint32 DX12SwapChain::Resize(uint2 newSize)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Resize SwapChain");
		m_Size = newSize;
		VAST_ASSERTF(m_Size.x != 0 && m_Size.y != 0, "Failed to resize swapchain. Invalid window size.");

		DestroyBackBuffers();
		{
			VAST_PROFILE_TRACE_SCOPE("gfx", "Resize Buffers");
			DXGI_SWAP_CHAIN_DESC scDesc = {};
			DX12Check(m_SwapChain->GetDesc(&scDesc));
			DX12Check(m_SwapChain->ResizeBuffers(NUM_BACK_BUFFERS, m_Size.x, m_Size.y, scDesc.BufferDesc.Format, scDesc.Flags));
		}
		CreateBackBuffers();

		return m_SwapChain->GetCurrentBackBufferIndex();
	}

	void DX12SwapChain::CreateBackBuffers()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create BackBuffers");
		VAST_LOG_TRACE("[gfx] [dx12] Creating backbuffers.");

		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			ID3D12Resource* backBuffer = nullptr;
			DX12Check(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			// Specialized initialization to fit specific BackBuffer needs.
			m_BackBuffers[i]->resource = backBuffer;
			m_BackBuffers[i]->state = D3D12_RESOURCE_STATE_PRESENT;
			m_BackBuffers[i]->rtv = m_Device.CreateBackBufferRTV(backBuffer, TranslateToDX12(m_BackBufferFormat));
			m_BackBuffers[i]->SetName(std::string("Back Buffer ") + std::to_string(i));
		}
	}

	void DX12SwapChain::DestroyBackBuffers()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy BackBuffers");
		VAST_LOG_TRACE("[gfx] [dx12] Destroying backbuffers.");

		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			VAST_ASSERT(m_BackBuffers[i]);
			m_Device.DestroyTexture(*m_BackBuffers[i]);
			m_BackBuffers[i]->Reset();
		}
	}

	bool DX12SwapChain::CheckTearingSupport()
	{
		BOOL allowTearing = FALSE;
		if (FAILED(m_Device.GetDXGIFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			allowTearing = FALSE;
		}
		return allowTearing == TRUE;
	}

}