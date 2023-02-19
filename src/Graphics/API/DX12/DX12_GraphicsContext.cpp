#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"
#include "Graphics/API/DX12/DX12_CommandList.h"

// TODO: Long relative path will make it hard to simply export an .exe, but it is a lot more comfortable for development.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "..\\..\\..\\..\\vendor\\dx12\\DirectXAgilitySDK\\bin\\x64\\"; }

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
	{
		VAST_PROFILE_FUNCTION();

		m_Device = MakePtr<DX12Device>();
		m_SwapChain = MakePtr<DX12SwapChain>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat, *m_Device);
		m_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*m_Device);
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_FUNCTION();

		m_Device->WaitForIdle();

		m_GraphicsCommandList = nullptr;
		m_SwapChain = nullptr;
		m_Device = nullptr;
	}

	void DX12GraphicsContext::BeginFrame()
	{
		m_Device->BeginFrame();
	}

	void DX12GraphicsContext::EndFrame()
	{
		m_Device->EndFrame();
	}

	void DX12GraphicsContext::Submit()
	{
		m_Device->SubmitCommandList(*m_GraphicsCommandList);
	}

	void DX12GraphicsContext::Present()
	{
		m_SwapChain->Present();
		m_Device->SignalEndOfFrame(QueueType::GRAPHICS);
	}

	Texture DX12GraphicsContext::GetCurrentBackBuffer() const
	{
		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();

		// TODO: Can we avoid memory allocation but retain similar flexibility?
		auto internalState = MakeRef<DX12Texture>();
		internalState->resource = backBuffer.resource;
		internalState->state = backBuffer.state;
		internalState->rtv = backBuffer .rtv;

		D3D12_RESOURCE_DESC desc = internalState->resource->GetDesc();
		// TODO: GetCopyableFootprints?

		Texture t;
		t.internalState = internalState;
		t.desc = TranslateFromDX12_Tex(desc);
		return t;
	}

	void DX12GraphicsContext::CreateBuffer(const BufferDesc& desc, Buffer* buffer, void* data /* = nullptr */)
	{
		VAST_ASSERT(buffer);

		auto internalState = m_Device->CreateBuffer(desc);
		if (data != nullptr)
		{
			internalState->SetMappedData(&data, sizeof(data));
		}

		buffer->internalState = internalState;
		buffer->desc = TranslateFromDX12_Buf(internalState->resource->GetDesc());
	}

	void DX12GraphicsContext::AddBarrier(GPUResource& resource, const ResourceState& state)
	{
		auto internalState = static_cast<DX12Resource*>(resource.internalState.get());
		m_GraphicsCommandList->AddBarrier(*internalState, TranslateToDX12(state));
	}

	void DX12GraphicsContext::FlushBarriers()
	{
		m_GraphicsCommandList->FlushBarriers();
	}

	void DX12GraphicsContext::Reset()
	{
		m_GraphicsCommandList->Reset();
	}

	void DX12GraphicsContext::ClearRenderTarget(const Texture& texture, float4 color)
	{
		auto internalState = static_cast<DX12Texture*>(texture.internalState.get());
 		m_GraphicsCommandList->ClearRenderTarget(*internalState, color);
	}
}