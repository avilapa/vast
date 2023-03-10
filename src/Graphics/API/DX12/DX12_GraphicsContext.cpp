#include "vastpch.h"
#include "Graphics/Resources.h"
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
		, m_Buffers(nullptr)
		, m_Textures(nullptr)
		, m_BuffersMarkedForDestruction({})
		, m_TexturesMarkedForDestruction({})
		, m_CurrentRT(nullptr)
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();

		m_Buffers = MakePtr<ResourceHandlePool<Buffer, DX12Buffer, NUM_BUFFERS>>();
		m_Textures = MakePtr<ResourceHandlePool<Texture, DX12Texture, NUM_TEXTURES>>();
		m_Pipelines = MakePtr<ResourceHandlePool<Pipeline, DX12Pipeline, NUM_PIPELINES>>();

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

		m_Device = nullptr;

		m_Buffers = nullptr;
		m_Textures = nullptr;
	}

	void DX12GraphicsContext::BeginFrame()
	{
		VAST_PROFILE_FUNCTION();
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
		VAST_PROFILE_FUNCTION();
		SubmitCommandList(*m_GraphicsCommandList);

		m_SwapChain->Present();

		SignalEndOfFrame(QueueType::GRAPHICS);
	}

	void DX12GraphicsContext::SubmitCommandList(DX12CommandList& cmdList)
	{
		switch (cmdList.GetCommandType())
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
			m_CommandQueues[IDX(QueueType::GRAPHICS)]->ExecuteCommandList(cmdList.GetCommandList());
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

	void DX12GraphicsContext::SetRenderTarget(const TextureHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		m_CurrentRT = m_Textures->LookupResource(h);
	}

	void DX12GraphicsContext::SetShaderResource(const BufferHandle& h, const ShaderResourceProxy& shaderResourceProxy)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(shaderResourceProxy.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetShaderResource(*buf, shaderResourceProxy.idx);
	}

	void DX12GraphicsContext::BeginRenderPass(const PipelineHandle& h)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());

		if (!m_CurrentRT)
		{
			m_CurrentRT = &m_SwapChain->GetCurrentBackBuffer();
		}

		m_GraphicsCommandList->AddBarrier(*m_CurrentRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_GraphicsCommandList->FlushBarriers();

		if (true) // TODO: Clear flags
		{
			float4 color = float4(0.6, 0.2, 0.9, 1.0);
			m_GraphicsCommandList->ClearRenderTarget(*m_CurrentRT, color);
		}

		auto pipeline = m_Pipelines->LookupResource(h);
		VAST_ASSERT(pipeline);

		m_GraphicsCommandList->SetPipeline(pipeline);
		m_GraphicsCommandList->SetRenderTargets(&m_CurrentRT, 1, nullptr);

		m_GraphicsCommandList->SetDefaultViewportAndScissor(m_SwapChain->GetSize()); // TODO: This shouldn't be here!
	}

	void DX12GraphicsContext::EndRenderPass()
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERTF(m_CurrentRT, "EndRenderPass called without matching BeginRenderPass call.");
		m_GraphicsCommandList->AddBarrier(*m_CurrentRT, D3D12_RESOURCE_STATE_PRESENT);
		m_GraphicsCommandList->FlushBarriers();

		m_GraphicsCommandList->SetPipeline(nullptr);
		m_CurrentRT = nullptr;
	}

	void DX12GraphicsContext::Draw(const uint32 vtxCount, const uint32 vtxStartLocation /* = 0 */)
	{
		m_GraphicsCommandList->Draw(vtxCount, vtxStartLocation);
	}

	TextureHandle DX12GraphicsContext::CreateTexture(const TextureDesc& desc, void* initialData /*= nullptr*/)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(m_Device);
		auto [h, tex] = m_Textures->AcquireResource();
		m_Device->CreateTexture(desc, tex);
		if (initialData != nullptr)
		{
			// TODO: Upload texture data.
		}
		return h;
	}

	BufferHandle DX12GraphicsContext::CreateBuffer(const BufferDesc& desc, void* initialData /*= nullptr*/, size_t dataSize /*= 0*/)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(m_Device);
		auto [h, buf] = m_Buffers->AcquireResource();
		m_Device->CreateBuffer(desc, buf);
		if (initialData != nullptr)
		{
			buf->SetMappedData(initialData, dataSize);
		}
		return h;
	}

	PipelineHandle DX12GraphicsContext::CreatePipeline(const PipelineDesc& desc)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(m_Device);
		auto [h, pipeline] = m_Pipelines->AcquireResource();
		m_Device->CreatePipeline(desc, pipeline);
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

	void DX12GraphicsContext::DestroyPipeline(const PipelineHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		m_PipelinesMarkedForDestruction[m_FrameId].push_back(h);
	}

	ShaderResourceProxy DX12GraphicsContext::LookupShaderResource(const PipelineHandle& h, const std::string& shaderResourceName)
	{
		VAST_ASSERT(h.IsValid());
		auto pipeline = m_Pipelines->LookupResource(h);
		if (pipeline && pipeline->resourceProxyTable->IsRegistered(shaderResourceName))
		{
			return pipeline->resourceProxyTable->LookupShaderResource(shaderResourceName);
		}
		return ShaderResourceProxy{ kInvalidShaderResourceProxy };
	}

	uint32 DX12GraphicsContext::GetBindlessHeapIndex(const BufferHandle& h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Buffers->LookupResource(h)->heapIdx;
	}

	void DX12GraphicsContext::ProcessDestructions(uint32 frameId)
	{
		VAST_PROFILE_FUNCTION();
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

		for (auto& h : m_PipelinesMarkedForDestruction[frameId])
		{
			DX12Pipeline* pipeline = m_Pipelines->LookupResource(h);
			VAST_ASSERT(pipeline);
			m_Device->DestroyPipeline(pipeline);
			m_Pipelines->FreeResource(h);
		}
		m_PipelinesMarkedForDestruction[frameId].clear();
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