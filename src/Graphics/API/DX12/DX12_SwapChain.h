#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

struct IDXGISwapChain4;

namespace vast::gfx
{
	class DX12Device;

	class DX12SwapChain
	{
	public:
		DX12SwapChain(const uint2& size, const Format& format, const Format& backBufferFormat,
			DX12Device& device, HWND windowHandle = ::GetActiveWindow());
		~DX12SwapChain();

		DX12Texture& GetCurrentBackBuffer() const;

		uint2 GetSize() const { return m_Size; }
		Format GetFormat() const {	return m_Format; }
		Format GetBackBufferFormat() const { return m_BackBufferFormat; }

		void Present();

		[[nodiscard]] uint32 Resize(uint2 newSize);

	private:
		void CreateBackBuffers();
		void DestroyBackBuffers();

		bool CheckTearingSupport();

		DX12Device& m_Device;

		IDXGISwapChain4* m_SwapChain;
		Array<Ptr<DX12Texture>, NUM_BACK_BUFFERS> m_BackBuffers;

		vast::uint2 m_Size;
		Format m_Format;
		Format m_BackBufferFormat;
	};

}