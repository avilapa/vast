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

	void DX12CommandList::Reset(uint32 frameId)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Reset Command List");
		m_CommandAllocators[frameId]->Reset();
		m_CommandList->Reset(m_CommandAllocators[frameId], nullptr);
		
		if (m_CommandType != D3D12_COMMAND_LIST_TYPE_COPY)
		{
			BindDescriptorHeaps(frameId);
		}
	}

	void DX12CommandList::AddBarrier(DX12Resource& resource, D3D12_RESOURCE_STATES newState)
	{
		if (m_NumQueuedBarriers >= MAX_QUEUED_BARRIERS)
		{
			FlushBarriers();
		}

		D3D12_RESOURCE_STATES oldState = resource.state;

		// TODO: Compute exceptions

		if (oldState != newState)
		{
			D3D12_RESOURCE_BARRIER& desc = m_ResourceBarrierQueue[m_NumQueuedBarriers++];

			desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			desc.Transition.pResource = resource.resource;
			desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			desc.Transition.StateBefore = oldState;
			desc.Transition.StateAfter = newState;
			desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

			resource.state = newState;

#if VAST_ENABLE_LOGGING_RESOURCE_BARRIERS
			// TODO: Vertex Buffer state will print as Constant Buffer since they share the state after cross translation.
			VAST_TRACE("[barrier] Added new barrier transition for resource '{}' ({} -> {})",
				resource.GetName(),
				std::string(g_ResourceStateNames[CountBits(IDX(TranslateFromDX12(oldState)))]),
				std::string(g_ResourceStateNames[CountBits(IDX(TranslateFromDX12(newState)))]));

			if (newState == D3D12_RESOURCE_STATE_GENERIC_READ)
			{
				VAST_WARNING("[barrier] [dx12] 'D3D12_RESOURCE_STATE_GENERIC_READ' state should be avoided where possible.");
			}
#endif
		}
		else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			D3D12_RESOURCE_BARRIER& desc = m_ResourceBarrierQueue[m_NumQueuedBarriers++];

			desc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			desc.UAV.pResource = resource.resource;

			// TODO: Logging for UAV barriers
		}
	}

	void DX12CommandList::FlushBarriers()
	{
		if (m_NumQueuedBarriers == 0)
			return;

		// TODO: Optimize barriers
// 		Array<D3D12_RESOURCE_BARRIER, MAX_QUEUED_BARRIERS> optimizedResourceBarrierQueue;
// 		for (uint32 i = 0; i < m_NumQueuedBarriers; ++i)
// 		{
// 
// 		}

		VAST_PROFILE_TRACE_SCOPE("gfx", "Flush Barriers");
#if VAST_ENABLE_LOGGING_RESOURCE_BARRIERS
		VAST_TRACE("[barrier] Flushing {} cached barrier transitions...", m_NumQueuedBarriers);
#endif
		m_CommandList->ResourceBarrier(m_NumQueuedBarriers, m_ResourceBarrierQueue.data());
		m_NumQueuedBarriers = 0;
	}

	void DX12CommandList::CopyResource(const DX12Resource& dst, const DX12Resource& src)
	{
		m_CommandList->CopyResource(dst.resource, src.resource);
	}

	void DX12CommandList::CopyBufferRegion(DX12Resource& dst, uint64 dstOffset, DX12Resource& src, uint64 srcOffset, uint64 numBytes)
	{
		m_CommandList->CopyBufferRegion(dst.resource, dstOffset, src.resource, srcOffset, numBytes);
	}

	void DX12CommandList::CopyTextureRegion(DX12Resource& dst, DX12Resource& src, size_t srcOffset, Array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, MAX_TEXTURE_SUBRESOURCE_COUNT>& subresourceLayouts, uint32 numSubresources)
	{
		for (uint32 i = 0; i < numSubresources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
			dstLocation.pResource = dst.resource;
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
			srcLocation.pResource = src.resource;
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLocation.PlacedFootprint = subresourceLayouts[i];
			srcLocation.PlacedFootprint.Offset += srcOffset;

			m_CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
		}
	}

	void DX12CommandList::BindDescriptorHeaps(uint32 frameId)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Descriptor Heaps");
		m_CurrentSRVDescriptorHeap = &m_Device.GetSRVDescriptorHeap(frameId);
		m_CurrentSRVDescriptorHeap->Reset();

		ID3D12DescriptorHeap* heapsToBind[]
		{
			m_Device.GetSRVDescriptorHeap(frameId).GetHeap(),
			m_Device.GetSamplerDescriptorHeap().GetHeap()
		};

		m_CommandList->SetDescriptorHeaps(NELEM(heapsToBind), heapsToBind);
	}

	void DX12CommandList::BeginQuery(const DX12QueryHeap& heap, D3D12_QUERY_TYPE type, uint32 idx)
	{
		if (type == D3D12_QUERY_TYPE_TIMESTAMP)
		{
			EndQuery(heap, type, idx);
		}
		else
		{
			m_CommandList->BeginQuery(heap.m_QueryHeap, type, idx);
		}
	}

	void DX12CommandList::EndQuery(const DX12QueryHeap& heap, D3D12_QUERY_TYPE type, uint32 idx)
	{
		m_CommandList->EndQuery(heap.m_QueryHeap, type, idx);
	}

	void DX12CommandList::ResolveQuery(const DX12QueryHeap& heap, D3D12_QUERY_TYPE type, uint32 idx, uint32 count, DX12Resource& dstRsc, uint64 dstOffset)
	{
		m_CommandList->ResolveQueryData(heap.m_QueryHeap, type, idx, count, dstRsc.resource, dstOffset);
	}

	//

	DX12GraphicsCommandList::DX12GraphicsCommandList(DX12Device& device)
		: DX12CommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT)
		, m_CurrentPipeline(nullptr)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Graphics Command List");
	}

	DX12GraphicsCommandList::~DX12GraphicsCommandList()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy Graphics Command List");
	}

	void DX12GraphicsCommandList::BeginRenderPass(const DX12RenderPassData& rpd)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Begin Render Pass");
		m_CommandList->BeginRenderPass(
			rpd.rtCount, 
			rpd.rtDesc, 
			(rpd.dsDesc.cpuDescriptor.ptr != 0) ? &rpd.dsDesc : nullptr, 
			D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES // TODO: This should be D3D12_RENDER_PASS_FLAG_NONE by default, test when we have some UAV example.
		);
	}

	void DX12GraphicsCommandList::EndRenderPass()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "End Render Pass");
		m_CommandList->EndRenderPass();
	}

	void DX12GraphicsCommandList::SetPipeline(DX12Pipeline* pipeline)
	{
		if (pipeline)
		{
			VAST_PROFILE_TRACE_SCOPE("gfx", "Set Pipeline State and Root Signature");
			m_CommandList->SetPipelineState(pipeline->pipelineState);
			m_CommandList->SetGraphicsRootSignature(pipeline->desc.pRootSignature);
		}
		m_CurrentPipeline = pipeline;
	}

	void DX12GraphicsCommandList::SetVertexBuffer(const DX12Buffer& buf, uint32 offset, uint32 stride)
	{
		VAST_ASSERTF(m_CurrentPipeline, "Attempted to bind vertex shader before setting a render pipeline.");

		auto rscDesc = buf.resource->GetDesc();

		D3D12_VERTEX_BUFFER_VIEW vbv = {};
		vbv.BufferLocation	= buf.gpuAddress + offset;
		vbv.SizeInBytes		= static_cast<uint32>(rscDesc.Width) - offset;
		vbv.StrideInBytes	= (stride != 0) ? stride : buf.stride;

		m_CommandList->IASetVertexBuffers(0, 1, &vbv); // TODO: Support setting multiple vertex buffers.
	}

	void DX12GraphicsCommandList::SetIndexBuffer(const DX12Buffer& buf, uint32 offset, DXGI_FORMAT format)
	{
		VAST_ASSERTF(m_CurrentPipeline, "Attempted to bind index shader before setting a render pipeline.");

		auto rscDesc = buf.resource->GetDesc();

		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.BufferLocation	= buf.gpuAddress + offset;
		ibv.SizeInBytes		= static_cast<uint32>(rscDesc.Width) - offset;
		ibv.Format			= (format != DXGI_FORMAT_UNKNOWN) ? format : rscDesc.Format;

		m_CommandList->IASetIndexBuffer(&ibv);
	}

	void DX12GraphicsCommandList::SetConstantBuffer(const DX12Buffer& buf, uint32 offset, uint32 slotIndex)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Set Constant Buffer");
		VAST_ASSERTF(m_CurrentPipeline, "Attempted to bind constant buffer before setting a render pipeline."); // TODO: What about global/per frame resources
		m_CommandList->SetGraphicsRootConstantBufferView(slotIndex, buf.gpuAddress + offset);
	}

	void DX12GraphicsCommandList::SetDescriptorTable(const D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Set Descriptor Table");
		VAST_ASSERTF(m_CurrentPipeline, "Attempted to bind descriptor table before setting a render pipeline."); // TODO: What about global/per frame resources
		m_CommandList->SetGraphicsRootDescriptorTable(m_CurrentPipeline->descriptorTableIndex, gpuHandle);
	}

	void DX12GraphicsCommandList::SetPushConstants(const void* data, const uint32 size)
	{
		VAST_ASSERT(data && size);
		VAST_ASSERTF(m_CurrentPipeline, "Attempted to bind push constant before setting a render pipeline.");
		VAST_ASSERTF(m_CurrentPipeline->pushConstantIndex != UINT8_MAX, "Currently set pipeline does not expect push constant.");
		m_CommandList->SetGraphicsRoot32BitConstants(m_CurrentPipeline->pushConstantIndex, size / sizeof(uint32), data, 0); // TODO: Support offset parameter.
	}

	void DX12GraphicsCommandList::SetDefaultViewportAndScissor(uint2 windowSize)
	{
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

	void DX12GraphicsCommandList::SetScissorRect(const D3D12_RECT& rect)
	{
		// TODO: Support setting multiple rects
		m_CommandList->RSSetScissorRects(1, &rect);
	}

	//

	DX12UploadCommandList::DX12UploadCommandList(DX12Device& device)
		: DX12CommandList(device, D3D12_COMMAND_LIST_TYPE_COPY)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Upload Command List");
		BufferDesc uploadBufferDesc;
		uploadBufferDesc.size = 10 * 1024 * 1024;
		uploadBufferDesc.usage = ResourceUsage::UPLOAD;

		BufferDesc uploadTextureDesc;
		uploadTextureDesc.size = 40 * 1024 * 1024;
		uploadTextureDesc.usage = ResourceUsage::UPLOAD;

		m_BufferUploadHeap = MakePtr<DX12Buffer>();
		m_Device.CreateBuffer(uploadBufferDesc, *m_BufferUploadHeap);

		m_TextureUploadHeap = MakePtr<DX12Buffer>();
		m_Device.CreateBuffer(uploadTextureDesc, *m_TextureUploadHeap);
	}

	DX12UploadCommandList::~DX12UploadCommandList()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy Upload Command List");
		VAST_ASSERT(m_BufferUploadHeap);
		m_Device.DestroyBuffer(*m_BufferUploadHeap);
		m_BufferUploadHeap = nullptr;

		VAST_ASSERT(m_TextureUploadHeap);
		m_Device.DestroyBuffer(*m_TextureUploadHeap);
		m_TextureUploadHeap = nullptr;
	}

	void DX12UploadCommandList::UploadBuffer(Ptr<BufferUpload> upload)
	{
		VAST_ASSERT(upload->size <= m_BufferUploadHeap->resource->GetDesc().Width);
		m_BufferUploads.push_back(std::move(upload));
	}
	
	void DX12UploadCommandList::UploadTexture(Ptr<TextureUpload> upload)
	{
		VAST_ASSERT(upload->size <= m_TextureUploadHeap->resource->GetDesc().Width);
		m_TextureUploads.push_back(std::move(upload));
	}

	void DX12UploadCommandList::ProcessUploads()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Process Uploads");
		const uint32 numBufferUploads = static_cast<uint32>(m_BufferUploads.size());
		uint32 numBuffersProcessed = 0;
		size_t bufferUploadHeapOffset = 0;

		for (numBuffersProcessed; numBuffersProcessed < numBufferUploads; ++numBuffersProcessed)
		{
			BufferUpload& currentUpload = *m_BufferUploads[numBuffersProcessed];

			if ((bufferUploadHeapOffset + currentUpload.size) > m_BufferUploadHeap->resource->GetDesc().Width)
			{
				break;
			}

			memcpy(m_BufferUploadHeap->data + bufferUploadHeapOffset, currentUpload.data.get(), currentUpload.size);
			CopyBufferRegion(*currentUpload.buf, 0, *m_BufferUploadHeap, bufferUploadHeapOffset, currentUpload.size);

			bufferUploadHeapOffset += currentUpload.size;
			m_BufferUploadsInProgress.push_back(currentUpload.buf);
		}

		const uint32 numTextureUploads = static_cast<uint32>(m_TextureUploads.size());
		uint32 numTexturesProcessed = 0;
		size_t textureUploadHeapOffset = 0;

		for (numTexturesProcessed; numTexturesProcessed < numTextureUploads; numTexturesProcessed++)
		{
			TextureUpload& currentUpload = *m_TextureUploads[numTexturesProcessed];

			if ((textureUploadHeapOffset + currentUpload.size) > m_TextureUploadHeap->resource->GetDesc().Width)
			{
				break;
			}

			memcpy(m_TextureUploadHeap->data + textureUploadHeapOffset, currentUpload.data.get(), currentUpload.size);
			CopyTextureRegion(*currentUpload.tex, *m_TextureUploadHeap, textureUploadHeapOffset, currentUpload.subresourceLayouts, currentUpload.numSubresources);

			textureUploadHeapOffset += currentUpload.size;
			textureUploadHeapOffset = AlignU64(textureUploadHeapOffset, 512);

			m_TextureUploadsInProgress.push_back(currentUpload.tex);
		}

		if (numBuffersProcessed > 0)
		{
			m_BufferUploads.erase(m_BufferUploads.begin(), m_BufferUploads.begin() + numBuffersProcessed);
		}

		if (numTexturesProcessed > 0)
		{
			m_TextureUploads.erase(m_TextureUploads.begin(), m_TextureUploads.begin() + numTexturesProcessed);
		}
	}

	void DX12UploadCommandList::ResolveProcessedUploads()
	{
		for (auto& buffer : m_BufferUploadsInProgress)
		{
			buffer->isReady = true;
		}

		for (auto& texture : m_TextureUploadsInProgress)
		{
			texture->isReady = true;
		}

		m_BufferUploadsInProgress.clear();
		m_TextureUploadsInProgress.clear();
	}

	//

	DX12QueryHeap::DX12QueryHeap(DX12Device& device, D3D12_QUERY_HEAP_TYPE heapType, uint32 queryCount)
	{
		D3D12_QUERY_HEAP_DESC heapDesc = {};
		heapDesc.Count = queryCount;
		heapDesc.Type = heapType;
		heapDesc.NodeMask = 0;

		DX12Check(device.GetDevice()->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&m_QueryHeap)));
	}

	DX12QueryHeap::~DX12QueryHeap()
	{
		DX12SafeRelease(m_QueryHeap);
	}

}