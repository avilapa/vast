#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

struct IDXGISwapChain4;

namespace vast
{
	constexpr uint32 NUM_BACK_BUFFERS = 3;

	class DX12Device;

	class DX12SwapChain
	{
	public:
		// TODO: HWND is platform specific!
		DX12SwapChain(const uint2& size, const TexFormat& format, const TexFormat& backBufferFormat,
			DX12Device& device, ID3D12CommandQueue& graphicsQueue, HWND windowHandle = ::GetActiveWindow());
		~DX12SwapChain();

		DX12Texture& GetCurrentBackBuffer() const;

		uint2 GetSize() const { return m_Size; }
		TexFormat GetFormat() const { return m_Format; }
		TexFormat GetBackBufferFormat() const { return m_BackBufferFormat; }

		void Present();

		uint32 Resize(uint2 newSize);

	private:
		void CreateBackBuffers();
		void DestroyBackBuffers();

		bool CheckTearingSupport();

		DX12Device& m_Device;

		IDXGISwapChain4* m_SwapChain;
		Array<Ptr<DX12Texture>, NUM_BACK_BUFFERS> m_BackBuffers;

		vast::uint2 m_Size;
		TexFormat m_Format;
		TexFormat m_BackBufferFormat;
	};

}