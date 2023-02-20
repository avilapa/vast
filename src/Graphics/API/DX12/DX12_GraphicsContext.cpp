#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_CommandList.h"

#include "Core/EventTypes.h"

// TODO: Long relative path will make it hard to simply export an .exe, but it is a lot more comfortable for development.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "..\\..\\..\\..\\vendor\\dx12\\DirectXAgilitySDK\\bin\\x64\\"; }

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
		: m_Device(nullptr)
		, m_SwapChain(nullptr)
		, m_GraphicsCommandList(nullptr)
		, m_TextureHandles(nullptr)
		, m_Textures(0)
		, m_CurrentRT(nullptr)
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();

		m_TextureHandles = MakePtr<HandlePool<Texture, NUM_TEXTURES>>();
		m_Textures.resize(NUM_TEXTURES);

		m_Device = MakePtr<DX12Device>();
		m_SwapChain = MakePtr<DX12SwapChain>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat, *m_Device);
		m_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*m_Device);

		VAST_SUBSCRIBE_TO_EVENT_DATA(WindowResizeEvent, DX12GraphicsContext::OnWindowResizeEvent);
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_FUNCTION();

		m_Device->WaitForIdle();

		m_GraphicsCommandList = nullptr;
		m_SwapChain = nullptr;
		m_Device = nullptr;

 		m_TextureHandles = nullptr;
	}

	void DX12GraphicsContext::BeginFrame()
	{
		m_FrameId = (m_FrameId + 1) % NUM_FRAMES_IN_FLIGHT;

		m_Device->BeginFrame(m_FrameId);
		m_GraphicsCommandList->Reset(m_FrameId);
	}

	void DX12GraphicsContext::EndFrame()
	{
		m_Device->SubmitCommandList(*m_GraphicsCommandList);
		m_Device->EndFrame();

		m_SwapChain->Present();

		m_Device->SignalEndOfFrame(m_FrameId, QueueType::GRAPHICS);
	}

	void DX12GraphicsContext::BeginRenderPass()
	{
		m_CurrentRT = &m_SwapChain->GetCurrentBackBuffer();
		BeginRenderPassInternal();
	}

	void DX12GraphicsContext::BeginRenderPass(const TextureHandle& h)
	{
		VAST_ASSERT(m_Textures[h.GetIdx()]);
		m_CurrentRT = m_Textures[h.GetIdx()].get();

		BeginRenderPassInternal();
	}

	void DX12GraphicsContext::BeginRenderPassInternal()
	{
		m_GraphicsCommandList->AddBarrier(*m_CurrentRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_GraphicsCommandList->FlushBarriers();
		
		if (true) // TODO: Clear flags
		{
			float4 color = float4(0.6, 0.2, 0.9, 1.0);
			m_GraphicsCommandList->ClearRenderTarget(*m_CurrentRT, color);
		}
	}

	void DX12GraphicsContext::EndRenderPass()
	{
		VAST_ASSERT(m_CurrentRT);
		m_GraphicsCommandList->AddBarrier(*m_CurrentRT, D3D12_RESOURCE_STATE_PRESENT);
		m_GraphicsCommandList->FlushBarriers();
	}

	void DX12GraphicsContext::OnWindowResizeEvent(WindowResizeEvent& event)
	{
		VAST_PROFILE_FUNCTION();

		const uint2 scSize = m_SwapChain->GetSize();

		if (event.m_WindowSize.x != scSize.x || event.m_WindowSize.y != scSize.y)
		{
			m_Device->WaitForIdle();
			m_FrameId = m_SwapChain->Resize(event.m_WindowSize);
		}

	}
}