#include "vastpch.h"
#include "Graphics/API/DX12/dx12_graphics_context.h"
#include "Graphics/API/DX12/dx12_device.h"
#include "Graphics/API/DX12/dx12_command_list.h"

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

	Texture& DX12GraphicsContext::GetCurrentBackBuffer() const
	{
		return m_Device->GetCurrentBackBuffer();
	}

	void DX12GraphicsContext::AddBarrier(Resource& resource, const ResourceState& state)
	{
		m_GraphicsCommandList->AddBarrier(resource, TranslateToDX12(state));
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
		m_GraphicsCommandList->ClearRenderTarget(texture, color);
	}
}