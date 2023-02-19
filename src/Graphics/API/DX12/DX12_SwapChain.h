#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

struct IDXGISwapChain4;

namespace vast::gfx
{
	class DX12Device;

	class DX12SwapChain
	{
	public:
		DX12SwapChain(const uint2& swapChainSize, const Format& swapChainFormat, const Format& backBufferFormat,
			DX12Device& device, HWND windowHandle = ::GetActiveWindow());
		~DX12SwapChain();

		DX12Texture& GetCurrentBackBuffer() const;

		uint2 GetSwapChainSize() const { return m_SwapChainSize; }
		Format GetSwapChainFormat() const {	return m_SwapChainFormat; }
		Format GetBackBufferFormat() const { return m_BackBufferFormat; }

		void Present();

	private:
		void OnWindowResizeEvent(WindowResizeEvent& event);

		void CreateBackBuffers();
		void DestroyBackBuffers();

		bool CheckTearingSupport();

		DX12Device& m_Device;

		IDXGISwapChain4* m_SwapChain;
		Array<Ptr<DX12Texture>, NUM_BACK_BUFFERS> m_BackBuffers;

		vast::uint2 m_SwapChainSize;
		Format m_SwapChainFormat;
		Format m_BackBufferFormat;
	};

}