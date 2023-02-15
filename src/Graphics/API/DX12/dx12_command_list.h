#pragma once

#include "Graphics/API/DX12/dx12_common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12RenderPassDescriptorHeap;

	struct Resource;
	struct Texture;

	class DX12CommandList
	{
	public:
		DX12CommandList(DX12Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
		~DX12CommandList();

		D3D12_COMMAND_LIST_TYPE GetCommandType() const;
		ID3D12GraphicsCommandList* GetCommandList() const;

		void Reset();
		void AddBarrier(Resource& resource, D3D12_RESOURCE_STATES newState);
		void FlushBarriers();

	protected:
		void BindDescriptorHeaps(uint32 frameId);

		DX12Device& m_Device;
		D3D12_COMMAND_LIST_TYPE m_CommandType;

		Array<ID3D12CommandAllocator*, NUM_FRAMES_IN_FLIGHT> m_CommandAllocators;
		ID3D12GraphicsCommandList4* m_CommandList;
		Array<D3D12_RESOURCE_BARRIER, MAX_QUEUED_BARRIERS> m_ResourceBarrierQueue;
		uint32 m_NumQueuedBarriers;

		DX12RenderPassDescriptorHeap* m_CurrentSRVDescriptorHeap;
	};

	class DX12GraphicsCommandList final : public DX12CommandList
	{
	public:
		DX12GraphicsCommandList(DX12Device& device);

		void SetDefaultViewportAndScissor(uint2 windowSize);

		void ClearRenderTarget(const Texture& rt, float4 color);
		void ClearDepthStencilTarget(const Texture& dst, float depth, uint8 stencil);
	};

}