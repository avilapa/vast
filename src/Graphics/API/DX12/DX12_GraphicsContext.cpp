#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"

#include "Core/EventTypes.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
		: m_Device(nullptr)
		, m_SwapChain(nullptr)
		, m_GraphicsCommandList(nullptr)
		, m_CommandQueues({ nullptr })
		, m_FrameFenceValues({ {0} })
		, m_Textures(nullptr)
 		, m_Buffers(nullptr)
		, m_Shaders(nullptr)
		, m_TexturesMarkedForDestruction({})
		, m_BuffersMarkedForDestruction({})
		, m_CurrentRT(nullptr)
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();

		m_Textures = MakePtr<ResourceHandlePool<DX12Texture, Texture, NUM_TEXTURES>>();
		m_Buffers = MakePtr<ResourceHandlePool<DX12Buffer, Buffer, NUM_BUFFERS>>();
		m_Shaders = MakePtr<ResourceHandlePool<DX12Shader, Shader, NUM_SHADERS>>();

		m_Device = MakePtr<DX12Device>();

		m_CommandQueues[IDX(QueueType::GRAPHICS)] = MakePtr<DX12CommandQueue>(m_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);

		m_SwapChain = MakePtr<DX12SwapChain>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat, 
			*m_Device, *m_CommandQueues[IDX(QueueType::GRAPHICS)]->GetQueue());

		m_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*m_Device);

		VAST_SUBSCRIBE_TO_EVENT_DATA(WindowResizeEvent, DX12GraphicsContext::OnWindowResizeEvent);
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_FUNCTION();

		WaitForIdle();

		m_GraphicsCommandList = nullptr;
		m_SwapChain = nullptr;

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			ProcessDestructions(i);
		}

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i] = nullptr;
		}

		// TODO: WaitForIdle?

		m_Device = nullptr;

		m_Textures = nullptr;
		m_Buffers = nullptr;
		m_Shaders = nullptr;
	}

	void DX12GraphicsContext::BeginFrame()
	{
		m_FrameId = (m_FrameId + 1) % NUM_FRAMES_IN_FLIGHT;

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i]->WaitForFenceValue(m_FrameFenceValues[i][m_FrameId]);
		}

		ProcessDestructions(m_FrameId);

		m_GraphicsCommandList->Reset(m_FrameId);
	}

	void DX12GraphicsContext::EndFrame()
	{
		SubmitCommandList(*m_GraphicsCommandList);

		m_SwapChain->Present();

		SignalEndOfFrame(QueueType::GRAPHICS);
	}

	void DX12GraphicsContext::SubmitCommandList(DX12CommandList& ctx)
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

	void DX12GraphicsContext::SignalEndOfFrame(const QueueType& type)
	{
		m_FrameFenceValues[IDX(type)][m_FrameId] = m_CommandQueues[IDX(type)]->SignalFence();
	}

	void DX12GraphicsContext::WaitForIdle()
	{
		VAST_PROFILE_FUNCTION();
		for (auto& q : m_CommandQueues)
		{
			q->WaitForIdle();
		}
	}

	void DX12GraphicsContext::BeginRenderPass()
	{
		m_CurrentRT = &m_SwapChain->GetCurrentBackBuffer();
		BeginRenderPassInternal();
	}

	void DX12GraphicsContext::BeginRenderPass(const TextureHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		m_CurrentRT = m_Textures->LookupResource(h);

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

	TextureHandle DX12GraphicsContext::CreateTexture(const TextureDesc& desc)
	{
		VAST_ASSERT(m_Device);
		auto [h, tex] = m_Textures->AcquireResource();
		m_Device->CreateTexture(desc, tex);
		return h;
	}

	BufferHandle DX12GraphicsContext::CreateBuffer(const BufferDesc& desc, void* initialData /*= nullptr*/, size_t dataSize /*= 0*/)
	{
		VAST_ASSERT(m_Device);
		auto [h, buf] = m_Buffers->AcquireResource();
		m_Device->CreateBuffer(desc, buf);
		buf->SetMappedData(initialData, dataSize);
		return h;
	}
	
	ShaderHandle DX12GraphicsContext::CreateShader(const ShaderDesc& desc)
	{
		VAST_ASSERT(m_Device);
		auto [h, shader] = m_Shaders->AcquireResource();
		m_Device->CreateShader(desc, shader);
		return h;
	}

	void DX12GraphicsContext::DestroyTexture(const TextureHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		m_TexturesMarkedForDestruction[m_FrameId].push_back(h);
	}

	void DX12GraphicsContext::DestroyBuffer(const BufferHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		m_BuffersMarkedForDestruction[m_FrameId].push_back(h);
	}

	void DX12GraphicsContext::DestroyShader(const ShaderHandle& h)
	{
		VAST_ASSERT(m_Device);
		VAST_ASSERT(h.IsValid());
		DX12Shader* shader = m_Shaders->LookupResource(h);
		VAST_ASSERT(shader);
		m_Device->DestroyShader(shader);
		m_Shaders->FreeResource(h);
	}

	uint32 DX12GraphicsContext::GetBindlessHeapIndex(const BufferHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Buffers->LookupResource(h)->heapIdx;
	}

	void DX12GraphicsContext::ProcessDestructions(uint32 frameId)
	{
		VAST_ASSERT(m_Device);

		for (auto& h : m_TexturesMarkedForDestruction[frameId])
		{
			DX12Texture* tex = m_Textures->LookupResource(h);
			VAST_ASSERT(tex);
			m_Device->DestroyTexture(tex);
			m_Textures->FreeResource(h);
		}
		m_TexturesMarkedForDestruction[frameId].clear();

		for (auto& h : m_BuffersMarkedForDestruction[frameId])
		{
			DX12Buffer* buf = m_Buffers->LookupResource(h);
			VAST_ASSERT(buf);
			m_Device->DestroyBuffer(buf);
			m_Buffers->FreeResource(h);
		}
		m_BuffersMarkedForDestruction[frameId].clear();

	}

	void DX12GraphicsContext::OnWindowResizeEvent(WindowResizeEvent& event)
	{
		VAST_PROFILE_FUNCTION();

		const uint2 scSize = m_SwapChain->GetSize();

		if (event.m_WindowSize.x != scSize.x || event.m_WindowSize.y != scSize.y)
		{
			WaitForIdle();
			m_FrameId = m_SwapChain->Resize(event.m_WindowSize);
		}

	}
}