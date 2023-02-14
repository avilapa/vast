#pragma once

#include "Graphics/API/DX12/dx12_graphics_context.h"
#include "Graphics/API/DX12/dx12_command_queue.h"
#include "Graphics/API/DX12/dx12_descriptors.h"

#include <dxgi1_6.h>

namespace vast::gfx
{
	// Graphics constants
	constexpr uint32 NUM_FRAMES_IN_FLIGHT = 2;
	constexpr uint32 NUM_BACK_BUFFERS = 3;
	constexpr uint32 NUM_RTV_STAGING_DESCRIPTORS = 256;

	enum class QueueType
	{
		GRAPHICS = 0,
		COUNT = 1,
	};

	struct Resource
	{
		enum class Type : bool
		{
			BUFFER = 0,
			TEXTURE = 1
		};

		Type m_Type = Type::BUFFER;

		ID3D12Resource* m_Resource = nullptr;
		// TODO: D3D12MA::Allocation* m_Allocation = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress = 0;
		D3D12_RESOURCE_DESC m_Desc = {};
		D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
		uint32 m_HeapIndex = UINT32_MAX; // Invalid value
		bool m_IsReady = false;
	};

	struct Texture : public Resource
	{
		Texture() : Resource() { m_Type = Type::TEXTURE; }

		DX12DescriptorHandle m_RTVDescriptor = {};
		DX12DescriptorHandle m_DSVDescriptor = {};
		DX12DescriptorHandle m_SRVDescriptor = {};
		DX12DescriptorHandle m_UAVDescriptor = {};
	};

	class DX12Device
	{
	public:
		DX12Device(const uint2& swapChainSize, const Format& swapChainFormat);
		~DX12Device();

		void BeginFrame();
		void EndFrame();
		void Present();
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

		void SignalEndOfFrame(const QueueType& type);

		// TODO: Could combine these in a single structure
		Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> m_CommandQueues;
		Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> m_FrameFenceValues;

		Ptr<DX12StagingDescriptorHeap> m_RTVStagingDescriptorHeap;

		Array<Ptr<Texture>, NUM_BACK_BUFFERS> m_BackBuffers;

		uint2 m_SwapChainSize;
		Format m_SwapChainFormat;
		uint32 m_FrameId;
	};

}