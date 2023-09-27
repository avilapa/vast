#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"

#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"

#include "Core/EventTypes.h"
#include "Graphics/Resources.h"

#include "dx12/DirectXTex/DirectXTex/DirectXTex.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace vast::gfx
{

	DX12GraphicsContext::DX12GraphicsContext(const GraphicsParams& params)
		: m_Device(nullptr)
		, m_SwapChain(nullptr)
		, m_GraphicsCommandList(nullptr)
		, m_UploadCommandLists({ nullptr })
		, m_CommandQueues({ nullptr })
		, m_FrameFenceValues({ {0} })
		, m_Buffers(nullptr)
		, m_Textures(nullptr)
		, m_Pipelines(nullptr)
	{
		CreateHandlePools();

		m_Buffers = MakePtr<ResourceHandler<DX12Buffer, Buffer, NUM_BUFFERS>>();
		m_Textures = MakePtr<ResourceHandler<DX12Texture, Texture, NUM_TEXTURES>>();
		m_Pipelines = MakePtr<ResourceHandler<DX12Pipeline, Pipeline, NUM_PIPELINES>>();

		m_Device = MakePtr<DX12Device>();

		m_CommandQueues[IDX(QueueType::GRAPHICS)] = MakePtr<DX12CommandQueue>(m_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_TimestampFrequency = double(m_CommandQueues[IDX(QueueType::GRAPHICS)]->GetTimestampFrequency());
		// TODO: Compute
 		m_CommandQueues[IDX(QueueType::UPLOAD)] = MakePtr<DX12CommandQueue>(m_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);

		m_SwapChain = MakePtr<DX12SwapChain>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat, 
			*m_Device, *m_CommandQueues[IDX(QueueType::GRAPHICS)]->GetQueue());

		m_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*m_Device);
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_UploadCommandLists[i] = MakePtr<DX12UploadCommandList>(*m_Device);
		}

		m_QueryHeap = MakePtr<DX12QueryHeap>(*m_Device, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, NUM_TIMESTAMP_QUERIES);
		CreateProfilingResources();

		CreateTempFrameAllocators();

		VAST_SUBSCRIBE_TO_EVENT("gfxctx", WindowResizeEvent, VAST_EVENT_HANDLER_CB(DX12GraphicsContext::OnWindowResizeEvent, WindowResizeEvent));
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy Graphics Context");
		WaitForIdle();

		DestroyTempFrameAllocators();

		m_QueryHeap = nullptr;
		DestroyProfilingResources();

		m_GraphicsCommandList = nullptr;
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_UploadCommandLists[i] = nullptr;
		}

		m_SwapChain = nullptr;

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i] = nullptr;
		}

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			ProcessDestructions(i);
		}

		m_Device = nullptr;

		m_Buffers = nullptr;
		m_Textures = nullptr;
		m_Pipelines = nullptr;

		DestroyHandlePools();
	}

	void DX12GraphicsContext::BeginFrame_Internal()
	{
		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			// Wait on fences from NUM_FRAMES_IN_FLIGHT frames ago
			m_CommandQueues[i]->WaitForFenceValue(m_FrameFenceValues[i][m_FrameId]);
		}

		m_UploadCommandLists[m_FrameId]->ResolveProcessedUploads();
		m_UploadCommandLists[m_FrameId]->Reset(m_FrameId);

		m_GraphicsCommandList->Reset(m_FrameId);
	}

	void DX12GraphicsContext::EndFrame_Internal()
	{
		m_UploadCommandLists[m_FrameId]->ProcessUploads();
		SubmitCommandList(*m_UploadCommandLists[m_FrameId]);
		SignalEndOfFrame(QueueType::UPLOAD);

		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		m_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
		m_GraphicsCommandList->FlushBarriers();

		SubmitCommandList(*m_GraphicsCommandList);
		m_SwapChain->Present();

		SignalEndOfFrame(QueueType::GRAPHICS);
	}

	void DX12GraphicsContext::FlushGPU()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Flush GPU Work");
		WaitForIdle();

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			ProcessDestructions(i);
		}
	}

	void DX12GraphicsContext::SubmitCommandList(DX12CommandList& cmdList)
	{
		switch (cmdList.GetCommandType())
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		{
			VAST_PROFILE_TRACE_SCOPE("gfx", "Execute Graphics Command List");
			m_CommandQueues[IDX(QueueType::GRAPHICS)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
		}
		// TODO: Compute
		case D3D12_COMMAND_LIST_TYPE_COPY:
		{
			VAST_PROFILE_TRACE_SCOPE("gfx", "Execute Upload Command List");
			m_CommandQueues[IDX(QueueType::UPLOAD)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
		}
		default:
			VAST_ASSERTF(0, "Unsupported context submit type.");
			break;
		}
	}

	void DX12GraphicsContext::SignalEndOfFrame(const QueueType type)
	{
		m_FrameFenceValues[IDX(type)][m_FrameId] = m_CommandQueues[IDX(type)]->SignalFence();
	}

	void DX12GraphicsContext::WaitForIdle()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Wait CPU on GPU");
		for (auto& q : m_CommandQueues)
		{
			q->WaitForIdle();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Render Passes
	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::BeginRenderPassToBackBuffer(PipelineHandle h, LoadOp loadOp /* = LoadOp::LOAD */, StoreOp storeOp /* = StoreOp::STORE */)
	{
		VAST_PROFILE_TRACE_BEGIN("gfx", "Render Pass");
		VAST_ASSERT(m_Pipelines && h.IsValid());
		m_GraphicsCommandList->SetPipeline(&m_Pipelines->LookupResource(h));

		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		m_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Setup backbuffer transitions
		// Note: No need for an end barrier since we only present once at the end of the frame.
		DX12RenderPassData rpd;
		rpd.rtCount = 1;
		rpd.rtDesc[0].cpuDescriptor = backBuffer.rtv.cpuHandle;
		rpd.rtDesc[0].BeginningAccess.Type = TranslateToDX12(loadOp);
		rpd.rtDesc[0].BeginningAccess.Clear.ClearValue = backBuffer.clearValue;
		rpd.rtDesc[0].EndingAccess.Type = TranslateToDX12(storeOp);

		BeginRenderPass_Internal(rpd);
		m_GraphicsCommandList->SetDefaultViewportAndScissor(m_SwapChain->GetSize());
	}

	void DX12GraphicsContext::BeginRenderPass(PipelineHandle h, const RenderPassTargets targets)
	{
		VAST_PROFILE_TRACE_BEGIN("gfx", "Render Pass");
		VAST_ASSERT(m_Pipelines && h.IsValid());
		DX12Pipeline& pipeline = m_Pipelines->LookupResource(h);
		m_GraphicsCommandList->SetPipeline(&pipeline);

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
			DX12Texture& rt = m_Textures->LookupResource(targets.rt[i].h);

			m_GraphicsCommandList->AddBarrier(rt, D3D12_RESOURCE_STATE_RENDER_TARGET);
			if (targets.rt[i].nextUsage != ResourceState::NONE)
			{
				m_RenderPassEndBarriers.push_back(std::make_pair(&rt, TranslateToDX12(targets.rt[i].nextUsage)));
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
			auto ds = m_Textures->LookupResource(targets.ds.h);

			m_GraphicsCommandList->AddBarrier(ds, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			if (targets.ds.nextUsage != ResourceState::NONE)
			{
				m_RenderPassEndBarriers.push_back(std::make_pair(&ds, TranslateToDX12(targets.ds.nextUsage)));
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

		BeginRenderPass_Internal(rpd);

		// TODO: Figure out something more robust than this.
		DX12Texture& rt = m_Textures->LookupResource(targets.rt[0].h);
		m_GraphicsCommandList->SetDefaultViewportAndScissor(uint2(rt.resource->GetDesc().Width, rt.resource->GetDesc().Height));
	}

	void DX12GraphicsContext::BeginRenderPass_Internal(const DX12RenderPassData& rpd)
	{
		m_GraphicsCommandList->FlushBarriers();

		VAST_ASSERTF(!m_bHasRenderPassBegun, "A render pass is already running.");
		m_GraphicsCommandList->BeginRenderPass(rpd);
		m_bHasRenderPassBegun = true;
	}

	void DX12GraphicsContext::EndRenderPass()
	{
		VAST_ASSERTF(m_bHasRenderPassBegun, "No render pass is currently running.");
		m_GraphicsCommandList->EndRenderPass();
		m_bHasRenderPassBegun = false;

		for (auto i : m_RenderPassEndBarriers)
		{
			m_GraphicsCommandList->AddBarrier(*i.first, i.second);
		}
		m_RenderPassEndBarriers.clear();

		m_GraphicsCommandList->SetPipeline(nullptr);
		VAST_PROFILE_TRACE_END("gfx", "Render Pass");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////\
	
	void DX12GraphicsContext::AddBarrier(BufferHandle h, ResourceState newState)
	{
		m_GraphicsCommandList->AddBarrier(m_Buffers->LookupResource(h), TranslateToDX12(newState));
	}

	void DX12GraphicsContext::AddBarrier(TextureHandle h, ResourceState newState)
	{
		m_GraphicsCommandList->AddBarrier(m_Textures->LookupResource(h), TranslateToDX12(newState));
	}
	
	void DX12GraphicsContext::FlushBarriers()
	{
		m_GraphicsCommandList->FlushBarriers();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Resource Binding
	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::BindVertexBuffer(BufferHandle h, uint32 offset /* = 0 */, uint32 stride /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Vertex Buffer");
		VAST_ASSERT(h.IsValid());
		m_GraphicsCommandList->SetVertexBuffer(m_Buffers->LookupResource(h), offset, stride);
	}

	void DX12GraphicsContext::BindIndexBuffer(BufferHandle h, uint32 offset /* = 0 */, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Index Buffer");
		VAST_ASSERT(h.IsValid());
		m_GraphicsCommandList->SetIndexBuffer(m_Buffers->LookupResource(h), offset, TranslateToDX12(format));
	}

	void DX12GraphicsContext::BindConstantBuffer(ShaderResourceProxy proxy, BufferHandle h, uint32 offset /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind Constant Buffer");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(proxy.IsValid());
		m_GraphicsCommandList->SetConstantBuffer(m_Buffers->LookupResource(h), offset, proxy.idx);
	}

	void DX12GraphicsContext::CopyToDescriptorTable(const DX12Descriptor& srcDesc)
	{
		// TODO TEMP: We should accumulate all SRV/UAV per shader space and combine them into a single descriptor table.
		DX12Descriptor blockStart = m_Device->GetSRVDescriptorHeap(m_FrameId).GetUserDescriptorBlockStart(1);
		m_Device->CopyDescriptorsSimple(1, blockStart.cpuHandle, srcDesc.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_GraphicsCommandList->SetDescriptorTable(blockStart.gpuHandle);
	}

	void DX12GraphicsContext::BindSRV(ShaderResourceProxy proxy, BufferHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind SRV");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		CopyToDescriptorTable(m_Buffers->LookupResource(h).srv);
	}

	void DX12GraphicsContext::BindSRV(ShaderResourceProxy proxy, TextureHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind SRV");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		CopyToDescriptorTable(m_Textures->LookupResource(h).srv);
	}
	
	void DX12GraphicsContext::BindUAV(ShaderResourceProxy proxy, TextureHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Bind UAV");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(proxy.IsValid());
		(void)proxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		CopyToDescriptorTable(m_Textures->LookupResource(h).uav);
	}

	void DX12GraphicsContext::SetPushConstants(const void* data, const uint32 size)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Set Push Constants");
		VAST_ASSERT(data && size);
		m_GraphicsCommandList->SetPushConstants(data, size);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::SetScissorRect(int4 rect)
	{
		const D3D12_RECT r = { (LONG)rect.x, (LONG)rect.y, (LONG)rect.z, (LONG)rect.w };
		m_GraphicsCommandList->SetScissorRect(r);
	}

	void DX12GraphicsContext::SetBlendFactor(float4 blend)
	{
		m_GraphicsCommandList->GetCommandList()->OMSetBlendFactor((float*)&blend);;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Draw Commands
	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::Draw(uint32 vtxCount, uint32 vtxStartLocation /* = 0 */)
	{
		DrawInstanced(vtxCount, 1, vtxStartLocation, 0);
	}

	void DX12GraphicsContext::DrawIndexed(uint32 idxCount, uint32 startIdxLocation /* = 0 */, uint32 baseVtxLocation /* = 0 */)
	{
		DrawIndexedInstanced(idxCount, 1, startIdxLocation, baseVtxLocation, 0);
	}

	void DX12GraphicsContext::DrawInstanced(uint32 vtxCountPerInstance, uint32 instCount, uint32 vtxStartLocation/* = 0 */, uint32 instStartLocation /* = 0 */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Draw Instanced");
		m_GraphicsCommandList->GetCommandList()->DrawInstanced(vtxCountPerInstance, instCount, vtxStartLocation, instStartLocation);
	}

	void DX12GraphicsContext::DrawIndexedInstanced(uint32 idxCountPerInst, uint32 instCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Draw Indexed Instanced");
		m_GraphicsCommandList->GetCommandList()->DrawIndexedInstanced(idxCountPerInst, instCount, startIdxLocation, baseVtxLocation, startInstLocation);
	}

	void DX12GraphicsContext::DrawFullscreenTriangle()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Draw Fullscreen Triangle");
		m_GraphicsCommandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		m_GraphicsCommandList->GetCommandList()->IASetIndexBuffer(nullptr);
		DrawInstanced(3, 1);
	}

	void DX12GraphicsContext::CreateBuffer_Internal(BufferHandle h, const BufferDesc& desc)
	{
		DX12Buffer& buf = m_Buffers->AcquireResource(h);
		m_Device->CreateBuffer(desc, buf);
		buf.SetName("Unnamed Buffer");
	}

	void DX12GraphicsContext::UpdateBuffer_Internal(BufferHandle h, const void* srcMem, size_t srcSize)
	{
		VAST_ASSERT(srcMem && srcSize);
		DX12Buffer& buf = m_Buffers->LookupResource(h);
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
			m_UploadCommandLists[m_FrameId]->UploadBuffer(std::move(upload));
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

	void DX12GraphicsContext::DestroyBuffer_Internal(BufferHandle h)
	{
		DX12Buffer& buf = m_Buffers->ReleaseResource(h);
		m_Device->DestroyBuffer(buf);
		buf.Reset();
	}

	void DX12GraphicsContext::CreateTexture_Internal(TextureHandle h, const TextureDesc& desc)
	{
		DX12Texture& tex = m_Textures->AcquireResource(h);
		m_Device->CreateTexture(desc, tex);
		tex.SetName("Unnamed Texture");
	}

	void DX12GraphicsContext::UpdateTexture_Internal(TextureHandle h, const void* srcMem)
	{
		VAST_ASSERT(srcMem);
		DX12Texture& tex = m_Textures->LookupResource(h);

		D3D12_RESOURCE_DESC desc = tex.resource->GetDesc();
		auto upload = std::make_unique<TextureUpload>();
		upload->tex = &tex;
		upload->numSubresources = desc.DepthOrArraySize * desc.MipLevels;

		UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
		uint64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
		m_Device->GetDevice()->GetCopyableFootprints(&desc, 0, upload->numSubresources, 0, upload->subresourceLayouts.data(), numRows, rowSizesInBytes, &upload->size);

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

		m_UploadCommandLists[m_FrameId]->UploadTexture(std::move(upload));
	}

	void DX12GraphicsContext::DestroyTexture_Internal(TextureHandle h)
	{
		DX12Texture& tex = m_Textures->ReleaseResource(h);
		m_Device->DestroyTexture(tex);
		tex.Reset();
	}

	void DX12GraphicsContext::CreatePipeline_Internal(PipelineHandle h, const PipelineDesc& desc)
	{
		DX12Pipeline& pipeline = m_Pipelines->AcquireResource(h);
		m_Device->CreatePipeline(desc, pipeline);
	}

	void DX12GraphicsContext::UpdatePipeline_Internal(const PipelineHandle h)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Update Pipeline");
		VAST_ASSERT(h.IsValid());
		WaitForIdle(); // TODO TEMP: We should cache pipelines that want to update on a given frame and reload them all at a safe place, but this does the job for now.
		m_Device->UpdatePipeline(m_Pipelines->LookupResource(h));
	}

	void DX12GraphicsContext::DestroyPipeline_Internal(PipelineHandle h)
	{
		DX12Pipeline& pipeline = m_Pipelines->ReleaseResource(h);
		m_Device->DestroyPipeline(pipeline);
		pipeline.Reset();
	}

	ShaderResourceProxy DX12GraphicsContext::LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName)
	{
		VAST_ASSERT(h.IsValid());
		DX12Pipeline& pipeline = m_Pipelines->LookupResource(h);
		if (pipeline.resourceProxyTable->IsRegistered(shaderResourceName))
		{
			return pipeline.resourceProxyTable->LookupShaderResource(shaderResourceName);
		}
		return ShaderResourceProxy{ kInvalidShaderResourceProxy };
	}

	uint2 DX12GraphicsContext::GetBackBufferSize() const
	{
		VAST_ASSERT(m_SwapChain);
		return m_SwapChain->GetSize();
	}

	float DX12GraphicsContext::GetBackBufferAspectRatio() const
	{
		auto size = GetBackBufferSize();
		return float(size.x) / float(size.y);
	}

	TexFormat DX12GraphicsContext::GetBackBufferFormat() const
	{
		VAST_ASSERT(m_SwapChain);
		return m_SwapChain->GetBackBufferFormat();
	}

	TexFormat DX12GraphicsContext::GetTextureFormat(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		// Note: Clear Value stores the format given on resource creation, while the format stored
		// in the descriptor is modified for depth/stencil targets to store a TYPELESS equivalent.
		// We could translate back to the original format, but since we have this...
		return TranslateFromDX12(m_Textures->LookupResource(h).clearValue.Format);
	}

	uint32 DX12GraphicsContext::GetBindlessIndex(DX12Descriptor& d)
	{
		VAST_ASSERT(d.IsValid() && d.bindlessIdx != kInvalidHeapIdx);
		return d.bindlessIdx;
	}

	uint32 DX12GraphicsContext::GetBindlessSRV(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return GetBindlessIndex(m_Buffers->LookupResource(h).srv);
	}

	uint32 DX12GraphicsContext::GetBindlessSRV(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return GetBindlessIndex(m_Textures->LookupResource(h).srv);
	}
	
	uint32 DX12GraphicsContext::GetBindlessUAV(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return GetBindlessIndex(m_Textures->LookupResource(h).uav);
	}

	bool DX12GraphicsContext::GetIsReady(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Buffers->LookupResource(h).isReady;
	}

	bool DX12GraphicsContext::GetIsReady(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Textures->LookupResource(h).isReady;
	}

	const uint8* DX12GraphicsContext::GetBufferData(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		DX12Buffer& buf = m_Buffers->LookupResource(h);
		VAST_ASSERTF(buf.usage != ResourceUsage::DEFAULT, "Data is not CPU-visible for this Buffer");
		return buf.data;
	}

	void DX12GraphicsContext::SetDebugName(BufferHandle h, const std::string& name)
	{
		VAST_ASSERT(h.IsValid());
		m_Buffers->LookupResource(h).SetName(name);
	}

	void DX12GraphicsContext::SetDebugName(TextureHandle h, const std::string& name)
	{
		VAST_ASSERT(h.IsValid());
		m_Textures->LookupResource(h).SetName(name);
	}

	void DX12GraphicsContext::BeginTimestamp_Internal(uint32 idx)
	{
		VAST_ASSERT(m_QueryHeap);
		m_GraphicsCommandList->BeginQuery(*m_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, idx);
	}

	void DX12GraphicsContext::EndTimestamp_Internal(uint32 idx)
	{
		VAST_ASSERT(m_QueryHeap);
		m_GraphicsCommandList->EndQuery(*m_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, idx);
	}

	void DX12GraphicsContext::CollectTimestamps_Internal(BufferHandle h, uint32 count)
	{
		VAST_ASSERT(m_QueryHeap);
		DX12Buffer& buf = m_Buffers->LookupResource(h);
		VAST_ASSERTF(buf.usage == ResourceUsage::READBACK, "This call requires readback buffer usage.");
		m_GraphicsCommandList->ResolveQuery(*m_QueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, count, buf, 0);
	}

	void DX12GraphicsContext::OnWindowResizeEvent(const WindowResizeEvent& event)
	{
		const uint2 scSize = m_SwapChain->GetSize();

		if (event.m_WindowSize.x != scSize.x || event.m_WindowSize.y != scSize.y)
		{
			WaitForIdle();
			// Note: Resize() here returns the BackBuffer index after resize, which is 0. We used
			// to assign this to m_FrameId as if resetting the count for these to be in sync, but
			// this appears to cause issues (would need to reset some buffered members), and also
			// it doesn't even make sense since they will lose sync after the first loop.
			m_SwapChain->Resize(event.m_WindowSize);
		}
	}

}