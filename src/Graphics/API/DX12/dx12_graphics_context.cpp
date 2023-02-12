#include "vastpch.h"
#include "Graphics/API/DX12/dx12_graphics_context.h"
#include "Graphics/API/DX12/dx12_device.h"

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
	{
		VAST_PROFILE_SCOPE("GFX", "DX12GraphicsContext::DX12GraphicsContext");

		m_Device = std::make_unique<DX12Device>(params.swapChainSize, params.swapChainFormat);
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_SCOPE("GFX", "DX12GraphicsContext::~DX12GraphicsContext");

		m_Device = nullptr;
	}

}