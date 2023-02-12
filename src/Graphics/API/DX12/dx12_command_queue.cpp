#include "vastpch.h"
#include "Graphics/API/DX12/dx12_command_queue.h"

namespace vast::gfx
{

	DX12CommandQueue::DX12CommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandType /* = D3D12_COMMAND_LIST_TYPE_DIRECT */)
		: m_CommandType(commandType)
		, m_Queue(nullptr)
		, m_Fence(nullptr)
		, m_NextFenceValue(1)
		, m_LastCompletedFenceValue(0)
		, m_FenceEventHandle(0)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = m_CommandType;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0; // Used for MGPU

		DX12Check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_Queue)));
		DX12Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

		m_Fence->Signal(m_LastCompletedFenceValue);

		m_FenceEventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		assert(m_FenceEventHandle != INVALID_HANDLE_VALUE && "Failed to create fence event.");
	}

	DX12CommandQueue::~DX12CommandQueue()
	{
		CloseHandle(m_FenceEventHandle);

		DX12SafeRelease(m_Fence);
		DX12SafeRelease(m_Queue);
	}

	bool DX12CommandQueue::IsFenceComplete(uint64 fenceValue)
	{
		if (fenceValue > m_LastCompletedFenceValue)
		{
			PollCurrentFenceValue();
		}

		return fenceValue <= m_LastCompletedFenceValue;
	}

	void DX12CommandQueue::WaitForFenceValue(uint64 fenceValue)
	{
		if (IsFenceComplete(fenceValue))
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lockGuard(m_EventMutex);

			m_Fence->SetEventOnCompletion(fenceValue, m_FenceEventHandle);
			WaitForSingleObjectEx(m_FenceEventHandle, INFINITE, false);
			m_LastCompletedFenceValue = fenceValue;
		}
	}

	void DX12CommandQueue::WaitForIdle()
	{
		WaitForFenceValue(m_NextFenceValue - 1); // TODO: How is this different to Flush?
	}

	void DX12CommandQueue::Flush()
	{
		WaitForFenceValue(SignalFence());
	}

	uint64 DX12CommandQueue::PollCurrentFenceValue()
	{
		m_LastCompletedFenceValue = (std::max)(m_LastCompletedFenceValue, m_Fence->GetCompletedValue());
		return m_LastCompletedFenceValue;
	}

	uint64 DX12CommandQueue::SignalFence()
	{
		std::lock_guard<std::mutex> lockGuard(m_FenceMutex);

		DX12Check(m_Queue->Signal(m_Fence, m_NextFenceValue));

		return m_NextFenceValue++;
	}

	uint64 DX12CommandQueue::ExecuteCommandList(ID3D12CommandList* commandList)
	{
		DX12Check(static_cast<ID3D12GraphicsCommandList*>(commandList)->Close());
		m_Queue->ExecuteCommandLists(1, &commandList);

		return SignalFence();
	}

}