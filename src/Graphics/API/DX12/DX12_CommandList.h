#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12Device;
	class DX12RenderPassDescriptorHeap;

	class DX12CommandList
	{
	public:
		DX12CommandList(DX12Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
		~DX12CommandList();

		D3D12_COMMAND_LIST_TYPE GetCommandType() const;
		ID3D12GraphicsCommandList* GetCommandList() const;

		void Reset(uint32 frameId);
		void AddBarrier(DX12Resource& resource, D3D12_RESOURCE_STATES newState);
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

		void SetPipeline(DX12Pipeline& pipeline);
		void SetRenderTargets(DX12Texture** rt, uint32 count, DX12Texture* ds);
		void SetPipelineResources(uint32 spaceId, DX12Buffer& cbv); // TODO TEMP: cbv

		void SetDefaultViewportAndScissor(uint2 windowSize);

		void ClearRenderTarget(const DX12Texture& rt, float4 color);
		void ClearDepthStencilTarget(const DX12Texture& dst, float depth, uint8 stencil);

		void DrawInstanced(uint32 vtxCountPerInstance, uint32 instCount, uint32 vtxStartLocation = 0, uint32 instStartLocation = 0);
		void Draw(uint32 vtxCount, uint32 vtxStartLocation = 0);
		void DrawFullscreenTriangle();

	private:
		DX12Pipeline* m_CurrentPipeline;
	};

}