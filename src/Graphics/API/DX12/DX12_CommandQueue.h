#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

namespace vast::gfx
{
	class DX12CommandQueue
	{
	public:
		DX12CommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT);
		~DX12CommandQueue();

		bool IsFenceComplete(uint64 fenceValue);
		void WaitForFenceValue(uint64 fenceValue);
		void WaitForIdle();
		void Flush();

		uint64 PollCurrentFenceValue();
		uint64 SignalFence();
		uint64 ExecuteCommandList(ID3D12CommandList* commandList);

		ID3D12CommandQueue* GetQueue() { return m_Queue; }

	private:
		D3D12_COMMAND_LIST_TYPE m_CommandType;
		ID3D12CommandQueue* m_Queue;
		ID3D12Fence* m_Fence;
		uint64 m_NextFenceValue;
		uint64 m_LastCompletedFenceValue;
		HANDLE m_FenceEventHandle;
		std::mutex m_FenceMutex;
		std::mutex m_EventMutex;
	};

}