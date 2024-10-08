#include "vastpch.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include <dxgi1_6.h>
#ifdef VAST_DEBUG
#include <dxgidebug.h>
#endif

namespace vast
{
	Arg g_EnableVSync("EnableVSync", true);
	Arg g_AllowTearing("AllowTearing", false);

	static_assert(NUM_BACK_BUFFERS <= DXGI_MAX_SWAP_CHAIN_BUFFERS);

	DX12SwapChain::DX12SwapChain(const uint2& size, const TexFormat& format, const TexFormat& backBufferFormat,
		DX12Device& device, ID3D12CommandQueue& graphicsQueue, WindowHandle windowHandle)
		: m_Device(device)
		, m_SwapChain(nullptr)
		, m_Size(size)
		, m_Format(format)
		, m_BackBufferFormat(backBufferFormat)
		, m_bEnableVSync()
		, m_bAllowTearing()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_LOG_TRACE("[gfx] [dx12] Creating swapchain.");

		// Process input arguments
		g_EnableVSync.Get(m_bEnableVSync);
		g_AllowTearing.Get(m_bAllowTearing);

		IDXGISwapChain1* swapChain = m_Device.CreateSwapChain(&graphicsQueue, windowHandle, NUM_BACK_BUFFERS, m_Size, TranslateToDX12(m_Format));
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
		VAST_PROFILE_TRACE_FUNCTION;

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
		VAST_PROFILE_TRACE_FUNCTION;

		const uint32 syncInterval = m_bEnableVSync ? 1 : 0;
		const uint32 presentFlags = (m_bAllowTearing && !m_bEnableVSync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		m_SwapChain->Present(syncInterval, presentFlags);
	}

	uint32 DX12SwapChain::Resize(uint2 newSize)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		m_Size = newSize;
		VAST_ASSERTF(m_Size.x != 0 && m_Size.y != 0, "Failed to resize swapchain. Invalid window size.");

		DestroyBackBuffers();
		{
			VAST_PROFILE_TRACE_SCOPE("ResizeBuffers (DX12)");
			DXGI_SWAP_CHAIN_DESC scDesc = {};
			DX12Check(m_SwapChain->GetDesc(&scDesc));
			DX12Check(m_SwapChain->ResizeBuffers(NUM_BACK_BUFFERS, m_Size.x, m_Size.y, scDesc.BufferDesc.Format, scDesc.Flags));
		}
		CreateBackBuffers();

		return m_SwapChain->GetCurrentBackBufferIndex();
	}

	void DX12SwapChain::CreateBackBuffers()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_LOG_TRACE("[gfx] [dx12] Creating backbuffers.");

		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			ID3D12Resource* backBuffer = nullptr;
			DX12Check(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
			m_BackBuffers[i]->resource = backBuffer;
			m_BackBuffers[i]->state	= D3D12_RESOURCE_STATE_PRESENT;
			m_BackBuffers[i]->rtv = m_Device.CreateBackBufferRTV(backBuffer, TranslateToDX12(m_BackBufferFormat));
			m_BackBuffers[i]->SetName(std::string("Back Buffer ") + std::to_string(i));
		}
	}

	void DX12SwapChain::DestroyBackBuffers()
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_LOG_TRACE("[gfx] [dx12] Destroying backbuffers.");

		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			VAST_ASSERT(m_BackBuffers[i]);
			m_Device.DestroyTexture(*m_BackBuffers[i]);
			m_BackBuffers[i]->Reset();
		}
	}

}