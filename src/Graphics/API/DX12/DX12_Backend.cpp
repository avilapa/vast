#include "vastpch.h"
#include "Graphics/GraphicsBackend.h"

// Note: Currently we only support one backend. When more backends are supported, this file needs 
// to be conditionally compiled in/out.

#include "Graphics/API/DX12/DX12_Common.h"
#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"

#include "dx12/DirectXTex/DirectXTex/DirectXTex.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace vast::gfx
{
	static uint32 s_FrameId = 0;

	static Ptr<DX12Device> s_Device = nullptr;
	static Ptr<DX12SwapChain> m_SwapChain = nullptr;
	static Ptr<DX12QueryHeap> s_QueryHeap = nullptr;
	static Ptr<DX12GraphicsCommandList> s_GraphicsCommandList = nullptr;
	// TODO: Async Compute (Ptr<DX12GraphicsCommandList> m_ComputeCommandList;)
	static Array<Ptr<DX12UploadCommandList>, NUM_FRAMES_IN_FLIGHT> s_UploadCommandLists = { nullptr };

	static Array<Ptr<DX12CommandQueue>, IDX(QueueType::COUNT)> s_CommandQueues = { nullptr };
	static Array<Array<uint64, NUM_FRAMES_IN_FLIGHT>, IDX(QueueType::COUNT)> s_FrameFenceValues = { {0} };

	using RenderPassEndBarrier = std::pair<DX12Texture*, D3D12_RESOURCE_STATES>;
	static Vector<RenderPassEndBarrier> s_RenderPassEndBarriers;

	static Ptr<ResourceHandler<DX12Buffer, Buffer, NUM_BUFFERS>> s_Buffers = nullptr;
	static Ptr<ResourceHandler<DX12Texture, Texture, NUM_TEXTURES>> s_Textures = nullptr;
	static Ptr<ResourceHandler<DX12Pipeline, Pipeline, NUM_PIPELINES>> s_Pipelines = nullptr;

	void GraphicsBackend::Init(const GraphicsParams& params)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Graphics Backend");
		VAST_LOG_TRACE("[gfx] [dx12] Initializing DX12 backend...");

		s_Buffers = MakePtr<ResourceHandler<DX12Buffer, Buffer, NUM_BUFFERS>>();
		s_Textures = MakePtr<ResourceHandler<DX12Texture, Texture, NUM_TEXTURES>>();
		s_Pipelines = MakePtr<ResourceHandler<DX12Pipeline, Pipeline, NUM_PIPELINES>>();

		s_Device = MakePtr<DX12Device>();

		s_CommandQueues[IDX(QueueType::GRAPHICS)] = MakePtr<DX12CommandQueue>(s_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		// TODO: Async Compute (s_CommandQueues[IDX(QueueType::COMPUTE)] = MakePtr<DX12CommandQueue>(s_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_COMPUTE);)
 		s_CommandQueues[IDX(QueueType::UPLOAD)] = MakePtr<DX12CommandQueue>(s_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);

		m_SwapChain = MakePtr<DX12SwapChain>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat, 
			*s_Device, *s_CommandQueues[IDX(QueueType::GRAPHICS)]->GetQueue());

		s_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*s_Device);
		// TODO: Async Compute (s_ComputeCommandList = MakePtr<DX12ComputeCommandList>(*s_Device);)
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			s_UploadCommandLists[i] = MakePtr<DX12UploadCommandList>(*s_Device);
		}

		s_QueryHeap = MakePtr<DX12QueryHeap>(*s_Device, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, NUM_TIMESTAMP_QUERIES);
	}

	void GraphicsBackend::Shutdown()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy Graphics Backend");
		WaitForIdle();

		s_QueryHeap = nullptr;
		s_GraphicsCommandList = nullptr;
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			s_UploadCommandLists[i] = nullptr;
		}

		m_SwapChain = nullptr;

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			s_CommandQueues[i] = nullptr;
		}

		s_Device = nullptr;

		s_Buffers = nullptr;
		s_Textures = nullptr;
		s_Pipelines = nullptr;
	}

	void GraphicsBackend::BeginFrame()
	{
		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			// Wait on fences from NUM_FRAMES_IN_FLIGHT frames ago
			s_CommandQueues[i]->WaitForFenceValue(s_FrameFenceValues[i][s_FrameId]);
		}

		s_UploadCommandLists[s_FrameId]->ResolveProcessedUploads();
		s_UploadCommandLists[s_FrameId]->Reset(s_FrameId);

		s_GraphicsCommandList->Reset(s_FrameId);
	}

	void SubmitCommandList(DX12CommandList& cmdList)
	{
		switch (cmdList.GetCommandType())
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		{
			VAST_PROFILE_TRACE_SCOPE("gfx", "Execute Graphics Command List");
			s_CommandQueues[IDX(QueueType::GRAPHICS)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
		}
		// TODO: Async Compute (case D3D12_COMMAND_LIST_TYPE_COMPUTE:)
		case D3D12_COMMAND_LIST_TYPE_COPY:
		{
			VAST_PROFILE_TRACE_SCOPE("gfx", "Execute Upload Command List");
			s_CommandQueues[IDX(QueueType::UPLOAD)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
		}
		default:
			VAST_ASSERTF(0, "Unsupported context submit type.");
			break;
		}
	}

	void SignalEndOfFrame(QueueType type)
	{
		s_FrameFenceValues[IDX(type)][s_FrameId] = s_CommandQueues[IDX(type)]->SignalFence();
	}

	void GraphicsBackend::EndFrame()
	{
		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		s_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
		s_GraphicsCommandList->FlushBarriers();
		SubmitCommandList(*s_GraphicsCommandList);

		// TODO: Async Compute. This probably needs to be exposed to the user to submit graphics and compute
		// command lists in the desired order.
		// (SubmitCommandList(*s_ComputeCommandList); SignalEndOfFrame(QueueType::COMPUTE);)

		s_UploadCommandLists[s_FrameId]->ProcessUploads();
		SubmitCommandList(*s_UploadCommandLists[s_FrameId]);
		SignalEndOfFrame(QueueType::UPLOAD);

		m_SwapChain->Present();
		SignalEndOfFrame(QueueType::GRAPHICS);

		s_FrameId = (s_FrameId + 1) % NUM_FRAMES_IN_FLIGHT;
	}

	uint32 GraphicsBackend::GetFrameId()
	{
		return s_FrameId;
	}

	void GraphicsBackend::BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp /* = LoadOp::LOAD */, StoreOp storeOp /* = StoreOp::STORE */)
	{
		VAST_ASSERT(s_Pipelines);
		s_GraphicsCommandList->SetPipeline(&s_Pipelines->LookupResource(h));

		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		s_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Setup backbuffer transitions
		// Note: No need for an end barrier since we only present once at the end of the frame.
		DX12RenderPassData rpd;
		rpd.rtCount = 1;
		rpd.rtDesc[0].cpuDescriptor = backBuffer.rtv.cpuHandle;
		rpd.rtDesc[0].BeginningAccess.Type = TranslateToDX12(loadOp);
		rpd.rtDesc[0].BeginningAccess.Clear.ClearValue = backBuffer.clearValue;
		rpd.rtDesc[0].EndingAccess.Type = TranslateToDX12(storeOp);

		s_GraphicsCommandList->FlushBarriers();
		s_GraphicsCommandList->BeginRenderPass(rpd);
		s_GraphicsCommandList->SetDefaultViewportAndScissor(m_SwapChain->GetSize());
	}

	void GraphicsBackend::BeginRenderPass(PipelineHandle h, const RenderPassTargets targets)
	{
		VAST_ASSERT(s_Pipelines);
		DX12Pipeline& pipeline = s_Pipelines->LookupResource(h);
		s_GraphicsCommandList->SetPipeline(&pipeline);

#ifdef VAST_DEBUG
		// Validate user bindings against PSO.
		uint32 rtCount = 0;
		for (uint32 i = 0; i < MAX_RENDERTARGETS; ++i)
		{
			if (targets.rt[i].h.IsValid())
				rtCount++;
		}
		VAST_ASSERT(rtCount == pipeline.desc.NumRenderTargets);
#endif

		// Set up transitions
		DX12RenderPassData rpd;
		rpd.rtCount = pipeline.desc.NumRenderTargets;
		for (uint32 i = 0; i < rpd.rtCount; ++i)
		{
			VAST_ASSERT(pipeline.desc.RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
			VAST_ASSERT(targets.rt[i].h.IsValid());
			DX12Texture& rt = s_Textures->LookupResource(targets.rt[i].h);

			s_GraphicsCommandList->AddBarrier(rt, D3D12_RESOURCE_STATE_RENDER_TARGET);
			if (targets.rt[i].nextUsage != ResourceState::NONE)
			{
				s_RenderPassEndBarriers.push_back(std::make_pair(&rt, TranslateToDX12(targets.rt[i].nextUsage)));
			}

			rpd.rtDesc[i].cpuDescriptor = rt.rtv.cpuHandle;
			rpd.rtDesc[i].BeginningAccess.Type = TranslateToDX12(targets.rt[i].loadOp);
			rpd.rtDesc[i].BeginningAccess.Clear.ClearValue = rt.clearValue;
			rpd.rtDesc[i].EndingAccess.Type = TranslateToDX12(targets.rt[i].storeOp);
			// TODO: Multisample support (EndingAccess.Resolve)
		}

		VAST_ASSERT(targets.ds.h.IsValid() == (pipeline.desc.DSVFormat != DXGI_FORMAT_UNKNOWN));
		if (targets.ds.h.IsValid())
		{
			auto ds = s_Textures->LookupResource(targets.ds.h);

			s_GraphicsCommandList->AddBarrier(ds, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			if (targets.ds.nextUsage != ResourceState::NONE)
			{
				s_RenderPassEndBarriers.push_back(std::make_pair(&ds, TranslateToDX12(targets.ds.nextUsage)));
			}

			rpd.dsDesc.cpuDescriptor = ds.dsv.cpuHandle;
			rpd.dsDesc.DepthBeginningAccess.Type = TranslateToDX12(targets.ds.loadOp);
			rpd.dsDesc.DepthBeginningAccess.Clear.ClearValue = ds.clearValue;
			rpd.dsDesc.DepthEndingAccess.Type = TranslateToDX12(targets.ds.storeOp);
			rpd.dsDesc.DepthEndingAccess.Resolve.pSrcResource = ds.resource;
			rpd.dsDesc.DepthEndingAccess.Resolve.PreserveResolveSource = rpd.dsDesc.DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			if (IsTexFormatStencil(TranslateFromDX12(ds.clearValue.Format)))
			{
				rpd.dsDesc.StencilBeginningAccess = rpd.dsDesc.DepthBeginningAccess;
				rpd.dsDesc.StencilEndingAccess = rpd.dsDesc.DepthEndingAccess;
			}
		}

		s_GraphicsCommandList->FlushBarriers();
		s_GraphicsCommandList->BeginRenderPass(rpd);

		// TODO: Figure out something more robust than this.
		DX12Texture& rt = s_Textures->LookupResource(targets.rt[0].h);
		s_GraphicsCommandList->SetDefaultViewportAndScissor(uint2(rt.resource->GetDesc().Width, rt.resource->GetDesc().Height));
	}

	void GraphicsBackend::EndRenderPass()
	{
		s_GraphicsCommandList->EndRenderPass();

		for (auto i : s_RenderPassEndBarriers)
		{
			s_GraphicsCommandList->AddBarrier(*i.first, i.second);
		}
		s_RenderPassEndBarriers.clear();

		s_GraphicsCommandList->SetPipeline(nullptr);
		VAST_PROFILE_TRACE_END("gfx", "Render Pass");
	}

	void GraphicsBackend::BindPipelineForCompute(PipelineHandle h)
	{
		VAST_ASSERT(s_Pipelines);
		DX12Pipeline& pipeline = s_Pipelines->LookupResource(h);
		s_GraphicsCommandList->SetPipeline(&pipeline);
	}

	void GraphicsBackend::WaitForIdle()
	{
		for (auto& q : s_CommandQueues)
		{
			q->WaitForIdle();
		}
	}

	//

	void GraphicsBackend::CreateBuffer(BufferHandle h, const BufferDesc& desc)
	{
		DX12Buffer& buf = s_Buffers->AcquireResource(h);
		s_Device->CreateBuffer(desc, buf);
		buf.SetName(desc.name);
	}

	void GraphicsBackend::CreateTexture(TextureHandle h, const TextureDesc& desc)
	{
		DX12Texture& tex = s_Textures->AcquireResource(h);
		s_Device->CreateTexture(desc, tex);
		tex.SetName(desc.name);
	}

	void GraphicsBackend::CreatePipeline(PipelineHandle h, const PipelineDesc& desc)
	{
		DX12Pipeline& pipeline = s_Pipelines->AcquireResource(h);
		s_Device->CreateGraphicsPipeline(desc, pipeline);
	}

	void GraphicsBackend::CreatePipeline(PipelineHandle h, const ShaderDesc& csDesc)
	{
		DX12Pipeline& pipeline = s_Pipelines->AcquireResource(h);
		s_Device->CreateComputePipeline(csDesc, pipeline);
	}

	void GraphicsBackend::UpdateBuffer(BufferHandle h, const void* srcMem, size_t srcSize)
	{
		VAST_ASSERT(srcMem && srcSize);
		DX12Buffer& buf = s_Buffers->LookupResource(h);
		// TODO: Check buffer does not have UAV
		// TODO: Offer option to use buffered approach to improve performance (how do we handle updating descriptors?)

		switch (buf.usage)
		{
		case ResourceUsage::DEFAULT:
		{
			Ptr<BufferUpload> upload = MakePtr<BufferUpload>();
			upload->buf = &buf;
			upload->data = MakePtr<uint8[]>(srcSize);
			upload->size = srcSize;
			memcpy(upload->data.get(), srcMem, srcSize);
			s_UploadCommandLists[s_FrameId]->UploadBuffer(std::move(upload));
			break;
		}
		case ResourceUsage::UPLOAD:
		{
			const auto dstSize = buf.resource->GetDesc().Width;
			VAST_ASSERT(srcSize > 0 && srcSize <= dstSize);
			uint8* dstMem = buf.data;
			memcpy(dstMem, srcMem, srcSize);
			break;
		}
		case ResourceUsage::READBACK:
		default:
			VAST_ASSERTF(0, "This path is not currently supported.");
			break;
		}
	}

	void GraphicsBackend::UpdateTexture(TextureHandle h, const void* srcMem)
	{
		VAST_ASSERT(srcMem);
		DX12Texture& tex = s_Textures->LookupResource(h);

		D3D12_RESOURCE_DESC desc = tex.resource->GetDesc();
		auto upload = std::make_unique<TextureUpload>();
		upload->tex = &tex;
		upload->numSubresources = desc.DepthOrArraySize * desc.MipLevels;

		UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
		uint64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
		s_Device->GetDevice()->GetCopyableFootprints(&desc, 0, upload->numSubresources, 0, upload->subresourceLayouts.data(), numRows, rowSizesInBytes, &upload->size);

		upload->data = std::make_unique<uint8[]>(upload->size);

		const uint8* srcMemory = reinterpret_cast<const uint8*>(srcMem);
		const uint64 srcTexelSize = DirectX::BitsPerPixel(desc.Format) / 8;

		for (uint64 arrayIdx = 0; arrayIdx < desc.DepthOrArraySize; ++arrayIdx)
		{
			uint64 mipWidth = desc.Width;
			for (uint64 mipIdx = 0; mipIdx < desc.MipLevels; ++mipIdx)
			{
				const uint64 subresourceIdx = mipIdx + (arrayIdx * desc.MipLevels);

				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subresourceLayout = upload->subresourceLayouts[subresourceIdx];
				const uint64 subresourceHeight = numRows[subresourceIdx];
				const uint64 subresourcePitch = AlignU32(subresourceLayout.Footprint.RowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
				const uint64 subresourceDepth = subresourceLayout.Footprint.Depth;
				const uint64 srcPitch = mipWidth * srcTexelSize;
				uint8* dstSubresourceMemory = upload->data.get() + subresourceLayout.Offset;

				for (uint64 sliceIdx = 0; sliceIdx < subresourceDepth; ++sliceIdx)
				{
					for (uint64 height = 0; height < subresourceHeight; ++height)
					{
						memcpy(dstSubresourceMemory, srcMemory, (std::min)(subresourcePitch, srcPitch));
						dstSubresourceMemory += subresourcePitch;
						srcMemory += srcPitch;
					}
				}

				mipWidth = (std::max)(mipWidth / 2, 1ull);
			}
		}

		s_UploadCommandLists[s_FrameId]->UploadTexture(std::move(upload));
	}

	void GraphicsBackend::ReloadShaders(PipelineHandle h)
	{
		s_Device->ReloadShaders(s_Pipelines->LookupResource(h));
	}

	void GraphicsBackend::DestroyBuffer(BufferHandle h)
	{
		DX12Buffer& buf = s_Buffers->ReleaseResource(h);
		s_Device->DestroyBuffer(buf);
		buf.Reset();
	}

	void GraphicsBackend::DestroyTexture(TextureHandle h)
	{
		DX12Texture& tex = s_Textures->ReleaseResource(h);
		s_Device->DestroyTexture(tex);
		tex.Reset();
	}

	void GraphicsBackend::DestroyPipeline(PipelineHandle h)
	{
		DX12Pipeline& pipeline = s_Pipelines->ReleaseResource(h);
		s_Device->DestroyPipeline(pipeline);
		pipeline.Reset();
	}

	ShaderResourceProxy GraphicsBackend::LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName)
	{
		VAST_ASSERT(h.IsValid());
		DX12Pipeline& pipeline = s_Pipelines->LookupResource(h);
		if (pipeline.resourceProxyTable.IsRegistered(shaderResourceName))
		{
			return pipeline.resourceProxyTable.LookupShaderResource(shaderResourceName);
		}
		return ShaderResourceProxy{ kInvalidShaderResourceProxy };
	}

	void GraphicsBackend::SetDebugName(BufferHandle h, const std::string& name)
	{
		s_Buffers->LookupResource(h).SetName(name);
	}

	void GraphicsBackend::SetDebugName(TextureHandle h, const std::string& name)
	{
		s_Textures->LookupResource(h).SetName(name);
	}

	const uint8* GraphicsBackend::GetBufferData(BufferHandle h)
	{
		DX12Buffer& buf = s_Buffers->LookupResource(h);
		VAST_ASSERTF(buf.usage != ResourceUsage::DEFAULT, "Data is not CPU-visible for this Buffer");
		return buf.data;
	}

	bool GraphicsBackend::GetIsReady(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return s_Buffers->LookupResource(h).isReady;
	}

	bool GraphicsBackend::GetIsReady(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return s_Textures->LookupResource(h).isReady;
	}

	TexFormat GraphicsBackend::GetTextureFormat(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		// Note: Clear Value stores the format given on resource creation, while the format stored
		// in the descriptor is modified for depth/stencil targets to store a TYPELESS equivalent.
		// We could translate back to the original format, but since we have this...
		return TranslateFromDX12(s_Textures->LookupResource(h).clearValue.Format);
	}

	//

	void GraphicsBackend::AddBarrier(BufferHandle h, ResourceState newState)
	{
		s_GraphicsCommandList->AddBarrier(s_Buffers->LookupResource(h), TranslateToDX12(newState));
	}

	void GraphicsBackend::AddBarrier(TextureHandle h, ResourceState newState)
	{
		s_GraphicsCommandList->AddBarrier(s_Textures->LookupResource(h), TranslateToDX12(newState));
	}
	
	void GraphicsBackend::FlushBarriers()
	{
		s_GraphicsCommandList->FlushBarriers();
	}

	//

	void GraphicsBackend::BindVertexBuffer(BufferHandle h, uint32 offset /* = 0 */, uint32 stride /* = 0 */)
	{
		s_GraphicsCommandList->SetVertexBuffer(s_Buffers->LookupResource(h), offset, stride);
	}

	void GraphicsBackend::BindIndexBuffer(BufferHandle h, uint32 offset /* = 0 */, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		s_GraphicsCommandList->SetIndexBuffer(s_Buffers->LookupResource(h), offset, TranslateToDX12(format));
	}

	void GraphicsBackend::BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset /* = 0 */)
	{
		s_GraphicsCommandList->SetConstantBuffer(s_Buffers->LookupResource(h), offset, proxy.idx);
	}

	void GraphicsBackend::SetPushConstants(const void* data, const uint32 size)
	{
		s_GraphicsCommandList->SetPushConstants(data, size);
	}

	void CopyToDescriptorTable(const DX12Descriptor& srcDesc)
	{
		VAST_ASSERT(srcDesc.IsValid());
		// TODO TEMP: We should accumulate all SRV/UAV per shader space and combine them into a single descriptor table.
		DX12Descriptor blockStart = s_Device->GetSRVDescriptorHeap(s_FrameId).GetUserDescriptorBlockStart(1);
		s_Device->CopyDescriptorsSimple(1, blockStart.cpuHandle, srcDesc.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		s_GraphicsCommandList->SetDescriptorTable(blockStart.gpuHandle);
	}

	void GraphicsBackend::BindSRV(BufferHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind SRV");
		CopyToDescriptorTable(s_Buffers->LookupResource(h).srv);
	}

	void GraphicsBackend::BindSRV(TextureHandle h)
	{
		CopyToDescriptorTable(s_Textures->LookupResource(h).srv);
	}
	
	void GraphicsBackend::BindUAV(TextureHandle h, uint32 mipLevel)
	{
		DX12Texture& tex = s_Textures->LookupResource(h);
		VAST_ASSERT(tex.uav.size() > mipLevel);
		CopyToDescriptorTable(tex.uav[mipLevel]);
	}

	uint32 GetBindlessIndex(DX12Descriptor& d)
	{
		VAST_ASSERT(d.IsValid() && d.bindlessIdx != kInvalidHeapIdx);
		return d.bindlessIdx;
	}

	uint32 GraphicsBackend::GetBindlessSRV(BufferHandle h)
	{
		return GetBindlessIndex(s_Buffers->LookupResource(h).srv);
	}

	uint32 GraphicsBackend::GetBindlessSRV(TextureHandle h)
	{
		return GetBindlessIndex(s_Textures->LookupResource(h).srv);
	}

	uint32 GraphicsBackend::GetBindlessUAV(TextureHandle h, uint32 mipLevel /* = 0 */)
	{
		DX12Texture& tex = s_Textures->LookupResource(h);
		VAST_ASSERT(tex.uav.size() > mipLevel);
		return GetBindlessIndex(tex.uav[mipLevel]);
	}

	//

	void GraphicsBackend::SetScissorRect(int4 rect)
	{
		const D3D12_RECT r = { (LONG)rect.x, (LONG)rect.y, (LONG)rect.z, (LONG)rect.w };
		s_GraphicsCommandList->SetScissorRect(r);
	}

	void GraphicsBackend::SetBlendFactor(float4 blend)
	{
		s_GraphicsCommandList->GetCommandList()->OMSetBlendFactor((float*)&blend);;
	}
	
	//

	void GraphicsBackend::DrawInstanced(uint32 vtxCountPerInstance, uint32 instCount, uint32 vtxStartLocation/* = 0 */, uint32 instStartLocation /* = 0 */)
	{
		s_GraphicsCommandList->GetCommandList()->DrawInstanced(vtxCountPerInstance, instCount, vtxStartLocation, instStartLocation);
	}

	void GraphicsBackend::DrawIndexedInstanced(uint32 idxCountPerInst, uint32 instCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation)
	{
		s_GraphicsCommandList->GetCommandList()->DrawIndexedInstanced(idxCountPerInst, instCount, startIdxLocation, baseVtxLocation, startInstLocation);
	}

	// TODO: Expose topology and index buffer and we can get rid of this.
	void GraphicsBackend::DrawFullscreenTriangle()
	{
		s_GraphicsCommandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		s_GraphicsCommandList->GetCommandList()->IASetIndexBuffer(nullptr);
		DrawInstanced(3, 1);
	}

	void GraphicsBackend::Dispatch(uint3 threadGroupCount)
	{
		s_GraphicsCommandList->Dispatch(threadGroupCount);
	}

	//

	void GraphicsBackend::ResizeSwapChainAndBackBuffers(uint2 newSize)
	{
		VAST_ASSERT(m_SwapChain);
		// Note: Resize() here returns the BackBuffer index after resize, which is 0. We used
		// to assign this to m_FrameId as if resetting the count for these to be in sync, but
		// this appears to cause issues (would need to reset some buffered members), and also
		// it doesn't even make sense since they will lose sync after the first loop.
		m_SwapChain->Resize(newSize);
	}

	uint2 GraphicsBackend::GetBackBufferSize()
	{
		VAST_ASSERT(m_SwapChain);
		return m_SwapChain->GetSize();
	}

	TexFormat GraphicsBackend::GetBackBufferFormat()
	{
		VAST_ASSERT(m_SwapChain);
		return m_SwapChain->GetBackBufferFormat();
	}

	//

	void GraphicsBackend::BeginTimestamp(uint32 idx)
	{
		VAST_ASSERT(s_QueryHeap);
		s_GraphicsCommandList->BeginQuery(*s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, idx);
	}

	void GraphicsBackend::EndTimestamp(uint32 idx)
	{
		VAST_ASSERT(s_QueryHeap);
		s_GraphicsCommandList->EndQuery(*s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, idx);
	}

	void GraphicsBackend::CollectTimestamps(BufferHandle h, uint32 count)
	{
		VAST_ASSERT(s_QueryHeap);
		DX12Buffer& buf = s_Buffers->LookupResource(h);
		VAST_ASSERTF(buf.usage == ResourceUsage::READBACK, "This call requires readback buffer usage.");
		s_GraphicsCommandList->ResolveQuery(*s_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, count, buf, 0);
	}

	double GraphicsBackend::GetTimestampFrequency()
	{
		VAST_ASSERT(s_CommandQueues[IDX(QueueType::GRAPHICS)]);
		return double(s_CommandQueues[IDX(QueueType::GRAPHICS)]->GetTimestampFrequency());
	}

}