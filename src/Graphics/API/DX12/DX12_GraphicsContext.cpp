#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_CommandList.h"

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
	{
		VAST_PROFILE_FUNCTION();

		m_Device = MakePtr<DX12Device>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat);
		m_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*m_Device);
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_FUNCTION();

		m_Device->WaitForIdle();

		m_GraphicsCommandList = nullptr;
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
		m_Device->Present();
	}

	Texture DX12GraphicsContext::GetCurrentBackBuffer() const
	{
		DX12Texture& backBuffer = m_Device->GetCurrentBackBuffer();

		// TODO: Can we avoid memory allocation but retain similar flexibility?
		auto internalState = MakeRef<DX12Texture>();
		internalState->m_Resource = backBuffer.m_Resource;
		internalState->m_Desc = backBuffer.m_Desc;
		internalState->m_State = backBuffer.m_State;
		internalState->m_RTVDescriptor = backBuffer .m_RTVDescriptor;

		D3D12_RESOURCE_DESC desc = internalState->m_Resource->GetDesc();
		// TODO: GetCopyableFootprints?

		Texture t;
		t.internalState = internalState;
		t.desc = TranslateFromDX12(desc);
		return t;
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