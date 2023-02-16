#include "vastpch.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_Descriptors.h"

namespace vast::gfx
{

	DX12CommandList::DX12CommandList(DX12Device& device, D3D12_COMMAND_LIST_TYPE commandListType)
		: m_Device(device)
		, m_CommandType(commandListType)
		, m_CommandAllocators({ nullptr })
		, m_CommandList(nullptr)
		, m_ResourceBarrierQueue({})
		, m_NumQueuedBarriers(0)
		, m_CurrentSRVDescriptorHeap(nullptr)
	{
		VAST_PROFILE_FUNCTION();

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DX12Check(m_Device.GetDevice()->CreateCommandAllocator(m_CommandType, IID_PPV_ARGS(&m_CommandAllocators[i])));
		}

		DX12Check(m_Device.GetDevice()->CreateCommandList1(0, m_CommandType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_CommandList)));
	}

	DX12CommandList::~DX12CommandList()
	{
		DX12SafeRelease(m_CommandList);

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DX12SafeRelease(m_CommandAllocators[i]);
		}
	}

	D3D12_COMMAND_LIST_TYPE DX12CommandList::GetCommandType() const
	{
		return m_CommandType;
	}

	ID3D12GraphicsCommandList* DX12CommandList::GetCommandList() const
	{
		return m_CommandList;
	}

	void DX12CommandList::Reset()
	{
		VAST_PROFILE_FUNCTION();

		uint32 frameId = m_Device.GetFrameId();

		m_CommandAllocators[frameId]->Reset();
		m_CommandList->Reset(m_CommandAllocators[frameId], nullptr);
		
		if (m_CommandType != D3D12_COMMAND_LIST_TYPE_COPY)
		{
			BindDescriptorHeaps(frameId);
		}
	}

	void DX12CommandList::AddBarrier(Resource& resource, D3D12_RESOURCE_STATES newState)
	{
		VAST_PROFILE_FUNCTION();

		if (m_NumQueuedBarriers >= MAX_QUEUED_BARRIERS)
		{
			FlushBarriers();
		}

		D3D12_RESOURCE_STATES oldState = resource.m_State;

		// TODO: Compute exceptions

		if (oldState != newState)
		{
			D3D12_RESOURCE_BARRIER& desc = m_ResourceBarrierQueue[m_NumQueuedBarriers++];

			desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			desc.Transition.pResource = resource.m_Resource;
			desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			desc.Transition.StateBefore = oldState;
			desc.Transition.StateAfter = newState;
			desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

			resource.m_State = newState;
		}
		else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			D3D12_RESOURCE_BARRIER& desc = m_ResourceBarrierQueue[m_NumQueuedBarriers++];

			desc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			desc.UAV.pResource = resource.m_Resource;
		}
	}

	void DX12CommandList::FlushBarriers()
	{
		VAST_PROFILE_FUNCTION();

		if (m_NumQueuedBarriers > 0)
		{
			m_CommandList->ResourceBarrier(m_NumQueuedBarriers, m_ResourceBarrierQueue.data());
			m_NumQueuedBarriers = 0;
		}
	}

	void DX12CommandList::BindDescriptorHeaps(uint32 frameId)
	{
		VAST_PROFILE_FUNCTION();

		m_CurrentSRVDescriptorHeap = &m_Device.GetSRVDescriptorHeap(frameId);
		m_CurrentSRVDescriptorHeap->Reset();

		const int count = 1;// 2;
		ID3D12DescriptorHeap* heapsToBind[count];
		heapsToBind[0] = m_Device.GetSRVDescriptorHeap(frameId).GetHeap();
 		//heapsToBind[1] = m_Device.GetSamplerDescriptorHeap().GetHeap();

		m_CommandList->SetDescriptorHeaps(count, heapsToBind);
	}

	//

	DX12GraphicsCommandList::DX12GraphicsCommandList(DX12Device& device)
		: DX12CommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
	}

	void DX12GraphicsCommandList::SetDefaultViewportAndScissor(uint2 windowSize)
	{
		VAST_PROFILE_FUNCTION();

		D3D12_VIEWPORT viewport;
		viewport.Width = static_cast<float>(windowSize.x);
		viewport.Height = static_cast<float>(windowSize.y);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;

		D3D12_RECT scissor;
		scissor.top = 0;
		scissor.left = 0;
		scissor.bottom = windowSize.y;
		scissor.right = windowSize.x;

		m_CommandList->RSSetViewports(1, &viewport);
		m_CommandList->RSSetScissorRects(1, &scissor);
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // TODO: This shouldn't be here!
	}

	void DX12GraphicsCommandList::ClearRenderTarget(const Texture& rt, float4 color)
	{
		VAST_PROFILE_FUNCTION();

		m_CommandList->ClearRenderTargetView(rt.m_RTVDescriptor.m_CPUHandle, (float*)&color, 0, nullptr);
	}

	void DX12GraphicsCommandList::ClearDepthStencilTarget(const Texture& dst, float depth, uint8 stencil)
	{
		VAST_PROFILE_FUNCTION();

		m_CommandList->ClearDepthStencilView(dst.m_DSVDescriptor.m_CPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
	}

}