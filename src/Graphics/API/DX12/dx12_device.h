#pragma once

#include "Graphics/API/DX12/dx12_graphics_context.h"
#include "Graphics/API/DX12/dx12_command_queue.h"

#include <dxgi1_6.h>

namespace vast::gfx
{

	class DX12Device
	{
	public:
		DX12Device(const uint2& swapChainSize, const Format& swapChainFormat);
		~DX12Device();

		void WaitForIdle();

	private:
		void CreateDeviceResources();
		void DestroyDeviceResources();
		void CreateSwapChainResources();
		void DestroySwapChainResources();
		void CreateBackBuffers();
		void DestroyBackBuffers();

		bool CheckTearingSupport();

		IDXGIFactory7* m_DXGIFactory;
		ID3D12Device5* m_Device;
		IDXGISwapChain4* m_SwapChain;

		uint2 m_SwapChainSize;
		Format m_SwapChainFormat;

		enum CommandQueueType : uint8
		{
			CMD_QUEUE_TYPE_GRAPHICS = 0,
			CMD_QUEUE_TYPE_COPY,
			CMD_QUEUE_TYPE_COUNT,
		};

		std::array<std::unique_ptr<DX12CommandQueue>, CMD_QUEUE_TYPE_COUNT> m_CommandQueues;

	};

}