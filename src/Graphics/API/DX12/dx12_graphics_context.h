#pragma once

#include "Graphics/graphics_context.h"

#include "dx12/DirectXAgilitySDK/include/d3d12.h"

namespace vast::gfx
{

	class DX12GraphicsContext final : public GraphicsContext
	{
	public:
		DX12GraphicsContext(const GraphicsParams& params);
		~DX12GraphicsContext();

	private:
		Ptr<class DX12Device> m_Device;
	};

	inline void DX12Check(HRESULT hr)
	{
		assert(SUCCEEDED(hr));
	}

	template <typename T>
	inline void DX12SafeRelease(T& p)
	{
		if (p)
		{
			p->Release();
			p = nullptr;
		}
	}
}