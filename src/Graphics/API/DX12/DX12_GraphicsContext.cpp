#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_CommandList.h"

#include "Core/EventTypes.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
		: m_Device(nullptr)
		, m_SwapChain(nullptr)
		, m_GraphicsCommandList(nullptr)
		, m_BufferHandles(nullptr)
		, m_Buffers(0)
		, m_TextureHandles(nullptr)
		, m_Textures(0)
		, m_ShaderHandles(nullptr)
		, m_Shaders(0)
		, m_CurrentRT(nullptr)
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();

		m_BufferHandles = MakePtr<HandlePool<Buffer, NUM_BUFFERS>>();
		m_Buffers.resize(NUM_BUFFERS);
		m_TextureHandles = MakePtr<HandlePool<Texture, NUM_TEXTURES>>();
		m_Textures.resize(NUM_TEXTURES);
		m_ShaderHandles = MakePtr<HandlePool<Shader, NUM_SHADERS>>();
		m_Shaders.resize(NUM_SHADERS);

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
		VAST_ASSERT(h.IsValid());
		m_CurrentRT = &m_Textures[h.GetIdx()];

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

	BufferHandle DX12GraphicsContext::CreateBuffer(const BufferDesc& desc, void* initialData /*= nullptr*/, size_t dataSize /*= 0*/)
	{
		VAST_ASSERT(m_Device);
		BufferHandle h = m_BufferHandles->Acquire();
		VAST_ASSERT(h.IsValid());
		DX12Buffer* buf = &m_Buffers[h.GetIdx()];
		m_Device->CreateBuffer(desc, buf);
		buf->SetMappedData(initialData, dataSize);
		return h;
	}

	TextureHandle DX12GraphicsContext::CreateTexture(const TextureDesc& desc)
	{
		VAST_ASSERT(m_Device);
		TextureHandle h = m_TextureHandles->Acquire();
		VAST_ASSERT(h.IsValid());
		DX12Texture* tex = &m_Textures[h.GetIdx()];
		m_Device->CreateTexture(desc, tex);
		return h;
	}
	
	ShaderHandle DX12GraphicsContext::CreateShader(const ShaderDesc& desc)
	{
		VAST_ASSERT(m_Device);
		ShaderHandle h = m_ShaderHandles->Acquire();
		VAST_ASSERT(h.IsValid());
		DX12Shader* shader = &m_Shaders[h.GetIdx()];
		m_Device->CreateShader(desc, shader);
		return h;
	}

	uint32 DX12GraphicsContext::GetBindlessHeapIndex(const BufferHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Buffers[h.GetIdx()].heapIdx;
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