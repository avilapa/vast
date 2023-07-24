#include "vastpch.h"
#include "Graphics/API/DX12/DX12_GraphicsContext.h"

#include "Graphics/API/DX12/DX12_CommandList.h"
#include "Graphics/API/DX12/DX12_CommandQueue.h"
#include "Graphics/API/DX12/DX12_Device.h"
#include "Graphics/API/DX12/DX12_SwapChain.h"

#include "Core/EventTypes.h"
#include "Graphics/Resources.h"

#include "dx12/DirectXTex/DirectXTex/DirectXTex.h"
#include <iostream>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

static const wchar_t* ASSETS_TEXTURES_PATH = L"../../assets/textures/";

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
		, m_BuffersMarkedForDestruction({})
		, m_TexturesMarkedForDestruction({})
		, m_PipelinesMarkedForDestruction({})
		, m_bHasFrameBegun(false)
		, m_bHasRenderPassBegun(false)
		, m_bHasBackBufferBeenRenderedToThisFrame(false)
	{
		VAST_PROFILE_SCOPE("gfx", "Create Graphics Context");
		m_Buffers   = MakePtr<HandlePool<DX12Buffer,   Buffer,   NUM_BUFFERS>>();
		m_Textures  = MakePtr<HandlePool<DX12Texture,  Texture,  NUM_TEXTURES>>();
		m_Pipelines = MakePtr<HandlePool<DX12Pipeline, Pipeline, NUM_PIPELINES>>();

		m_Device = MakePtr<DX12Device>();

		m_CommandQueues[IDX(QueueType::GRAPHICS)] = MakePtr<DX12CommandQueue>(m_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		// TODO: Compute
 		m_CommandQueues[IDX(QueueType::UPLOAD)] = MakePtr<DX12CommandQueue>(m_Device->GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);

		m_SwapChain = MakePtr<DX12SwapChain>(params.swapChainSize, params.swapChainFormat, params.backBufferFormat, 
			*m_Device, *m_CommandQueues[IDX(QueueType::GRAPHICS)]->GetQueue());

		m_GraphicsCommandList = MakePtr<DX12GraphicsCommandList>(*m_Device);

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_UploadCommandLists[i] = MakePtr<DX12UploadCommandList>(*m_Device);
		}

		CreateTempFrameAllocators();

		VAST_SUBSCRIBE_TO_EVENT("gfxctx", WindowResizeEvent, VAST_EVENT_HANDLER_CB(DX12GraphicsContext::OnWindowResizeEvent, WindowResizeEvent));
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_SCOPE("gfx", "Destroy Graphics Context");
		WaitForIdle();
		
		DestroyTempFrameAllocators();

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

		m_Buffers = nullptr;
		m_Textures = nullptr;
		m_Pipelines = nullptr;

		m_Device = nullptr;
	}

	void DX12GraphicsContext::BeginFrame()
	{
		VAST_PROFILE_SCOPE("gfx", "Begin Frame");
		VAST_ASSERTF(!m_bHasFrameBegun, "A frame is already running");
		m_bHasFrameBegun = true;

		m_FrameId = (m_FrameId + 1) % NUM_FRAMES_IN_FLIGHT;

		for (uint32 i = 0; i < IDX(QueueType::COUNT); ++i)
		{
			m_CommandQueues[i]->WaitForFenceValue(m_FrameFenceValues[i][m_FrameId]);
		}

		ProcessDestructions(m_FrameId);

		m_TempFrameAllocators[m_FrameId].Reset();

		m_UploadCommandLists[m_FrameId]->ResolveProcessedUploads();
		m_UploadCommandLists[m_FrameId]->Reset(m_FrameId);

		m_GraphicsCommandList->Reset(m_FrameId);
	}

	void DX12GraphicsContext::EndFrame()
	{
		VAST_PROFILE_SCOPE("gfx", "End Frame");
		VAST_ASSERTF(m_bHasFrameBegun, "No frame is currently running.");

		m_UploadCommandLists[m_FrameId]->ProcessUploads();
		SubmitCommandList(*m_UploadCommandLists[m_FrameId]);
		SignalEndOfFrame(QueueType::UPLOAD);

		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		m_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
		m_GraphicsCommandList->FlushBarriers();

		SubmitCommandList(*m_GraphicsCommandList);
		m_SwapChain->Present();
		m_bHasBackBufferBeenRenderedToThisFrame = false;
		SignalEndOfFrame(QueueType::GRAPHICS);
		m_bHasFrameBegun = false;
	}

	bool DX12GraphicsContext::IsInFrame() const
	{
		return m_bHasFrameBegun;
	}

	void DX12GraphicsContext::FlushGPU()
	{
		VAST_PROFILE_SCOPE("gfx", "Flush GPU Work");
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
			VAST_PROFILE_SCOPE("gfx", "Execute Graphics Command List");
			m_CommandQueues[IDX(QueueType::GRAPHICS)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
		}
		// TODO: Compute
		case D3D12_COMMAND_LIST_TYPE_COPY:
		{
			VAST_PROFILE_SCOPE("gfx", "Execute Upload Command List");
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
		VAST_PROFILE_SCOPE("gfx", "Wait CPU on GPU");
		for (auto& q : m_CommandQueues)
		{
			q->WaitForIdle();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Render Passes
	///////////////////////////////////////////////////////////////////////////////////////////////

	DX12RenderPassData DX12GraphicsContext::SetupBackBufferRenderPassBarrierTransitions(LoadOp loadOp, StoreOp storeOp)
	{
		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		m_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Note: No need for an end barrier since we only present once at the end of the frame.
		DX12RenderPassData rpd;
		rpd.rtCount = 1;
		rpd.rtDesc[0].cpuDescriptor = backBuffer.rtv.cpuHandle;
		rpd.rtDesc[0].BeginningAccess.Type = TranslateToDX12(loadOp);
		rpd.rtDesc[0].BeginningAccess.Clear.ClearValue = backBuffer.clearValue;
		rpd.rtDesc[0].EndingAccess.Type = TranslateToDX12(storeOp);

		return rpd;
	}

	DX12RenderPassData DX12GraphicsContext::SetupCommonRenderPassBarrierTransitions(DX12Pipeline* pipeline, RenderPassTargets targets)
	{
		VAST_PROFILE_SCOPE("gfx", "Setup Barriers (RT)");
		DX12RenderPassData rpd;
		rpd.rtCount = pipeline->desc.NumRenderTargets;
		for (uint32 i = 0; i < rpd.rtCount; ++i)
		{
			VAST_ASSERT(pipeline->desc.RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
			VAST_ASSERT(targets.rt[i].h.IsValid());
			auto rt = m_Textures->LookupResource(targets.rt[i].h);
			VAST_ASSERT(rt);

			m_GraphicsCommandList->AddBarrier(*rt, D3D12_RESOURCE_STATE_RENDER_TARGET);
			if (targets.rt[i].nextUsage != ResourceState::NONE)
			{
				m_RenderPassEndBarriers.push_back(std::make_pair(rt, TranslateToDX12(targets.rt[i].nextUsage)));
			}

			rpd.rtDesc[i].cpuDescriptor = rt->rtv.cpuHandle;
			rpd.rtDesc[i].BeginningAccess.Type = TranslateToDX12(targets.rt[i].loadOp);
			rpd.rtDesc[i].BeginningAccess.Clear.ClearValue = rt->clearValue;
			rpd.rtDesc[i].EndingAccess.Type = TranslateToDX12(targets.rt[i].storeOp);
			// TODO: Multisample support (EndingAccess.Resolve)
		}

		VAST_ASSERT(targets.ds.h.IsValid() == (pipeline->desc.DSVFormat != DXGI_FORMAT_UNKNOWN));
		if (targets.ds.h.IsValid())
		{
			auto ds = m_Textures->LookupResource(targets.ds.h);
			VAST_ASSERT(ds);

			m_GraphicsCommandList->AddBarrier(*ds, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			if (targets.ds.nextUsage != ResourceState::NONE)
			{
				m_RenderPassEndBarriers.push_back(std::make_pair(ds, TranslateToDX12(targets.ds.nextUsage)));
			}

			rpd.dsDesc.cpuDescriptor = ds->dsv.cpuHandle;
			rpd.dsDesc.DepthBeginningAccess.Type = TranslateToDX12(targets.ds.loadOp);
			rpd.dsDesc.DepthBeginningAccess.Clear.ClearValue = ds->clearValue;
			rpd.dsDesc.DepthEndingAccess.Type = TranslateToDX12(targets.ds.storeOp);
			rpd.dsDesc.DepthEndingAccess.Resolve.pSrcResource = ds->resource;
			rpd.dsDesc.DepthEndingAccess.Resolve.PreserveResolveSource = rpd.dsDesc.DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			if (IsTexFormatStencil(TranslateFromDX12(ds->clearValue.Format)))
			{
				rpd.dsDesc.StencilBeginningAccess = rpd.dsDesc.DepthBeginningAccess;
				rpd.dsDesc.StencilEndingAccess = rpd.dsDesc.DepthEndingAccess;
			}
		}
		return rpd;
	}

	void DX12GraphicsContext::BeginRenderPass_Internal(const DX12RenderPassData& rpd)
	{
		m_GraphicsCommandList->FlushBarriers();

		VAST_ASSERTF(!m_bHasRenderPassBegun, "A render pass is already running.");
		m_GraphicsCommandList->BeginRenderPass(rpd);
		m_bHasRenderPassBegun = true;

		m_GraphicsCommandList->SetDefaultViewportAndScissor(m_SwapChain->GetSize()); // TODO: This shouldn't be here!
	}

	void DX12GraphicsContext::BeginRenderPassToBackBuffer_Internal(DX12Pipeline* pipeline, LoadOp loadOp, StoreOp storeOp)
	{
		VAST_ASSERT(pipeline);
		m_GraphicsCommandList->SetPipeline(pipeline);

		DX12RenderPassData rdp = SetupBackBufferRenderPassBarrierTransitions(loadOp, storeOp);
		BeginRenderPass_Internal(rdp);
	}

	void DX12GraphicsContext::BeginRenderPassToBackBuffer(const PipelineHandle h, LoadOp loadOp /* = LoadOp::LOAD */, StoreOp storeOp /* = StoreOp::STORE */)
	{
		VAST_PROFILE_BEGIN("gfx", "Render Pass");
		VAST_ASSERT(m_Pipelines && h.IsValid());
		BeginRenderPassToBackBuffer_Internal(m_Pipelines->LookupResource(h), loadOp, storeOp);
	}

	void DX12GraphicsContext::ValidateRenderPassTargets(DX12Pipeline* pipeline, RenderPassTargets targets) const
	{
		// Validate user bindings against PSO.
		uint32 rtCount = 0;
		for (uint32 i = 0; i < MAX_RENDERTARGETS; ++i)
		{
			if (targets.rt[i].h.IsValid())
				rtCount++;
		}
		VAST_ASSERT(rtCount == pipeline->desc.NumRenderTargets);
	}

	void DX12GraphicsContext::BeginRenderPass(const PipelineHandle h, const RenderPassTargets targets)
	{
		VAST_PROFILE_BEGIN("gfx", "Render Pass");
		VAST_ASSERT(m_Pipelines && m_Textures && h.IsValid());
		auto pipeline = m_Pipelines->LookupResource(h);
		VAST_ASSERT(pipeline);
		m_GraphicsCommandList->SetPipeline(pipeline);

#ifdef VAST_DEBUG
		ValidateRenderPassTargets(pipeline, targets);
#endif
		DX12RenderPassData rdp = SetupCommonRenderPassBarrierTransitions(pipeline, targets);
		BeginRenderPass_Internal(rdp);
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
		VAST_PROFILE_END("gfx", "Render Pass");
	}

	bool DX12GraphicsContext::IsInRenderPass() const
	{
		return m_bHasRenderPassBegun;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Resource Binding
	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::SetVertexBuffer(const BufferHandle h, uint32 offset /* = 0 */, uint32 stride /* = 0 */)
	{
		VAST_PROFILE_SCOPE("gfx", "Set Vertex Buffer");
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetVertexBuffer(*buf, offset, stride);
	}

	void DX12GraphicsContext::SetIndexBuffer(const BufferHandle h, uint32 offset /* = 0 */, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		VAST_PROFILE_SCOPE("gfx", "Set Index Buffer");
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetIndexBuffer(*buf, offset, TranslateToDX12(format));
	}

	void DX12GraphicsContext::SetConstantBufferView(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy)
	{
		VAST_PROFILE_SCOPE("gfx", "Set Shader Resource");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(shaderResourceProxy.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetConstantBuffer(*buf, 0, shaderResourceProxy.idx);
	}

	void DX12GraphicsContext::SetShaderResourceView(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy)
	{
		VAST_PROFILE_SCOPE("gfx", "Set Shader Resource");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(shaderResourceProxy.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		(void)shaderResourceProxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		SetShaderResourceView_Internal(buf->srv);
	}

	void DX12GraphicsContext::SetShaderResourceView(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy)
	{
		VAST_PROFILE_SCOPE("gfx", "Set Shader Resource");
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(shaderResourceProxy.IsValid());
		auto tex = m_Textures->LookupResource(h);
		VAST_ASSERT(tex);
		(void)shaderResourceProxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		SetShaderResourceView_Internal(tex->srv);
	}

	void DX12GraphicsContext::SetShaderResourceView_Internal(const DX12Descriptor& srv)
	{
		// TODO TEMP: We should accumulate all SRV/UAV per shader space and combine them into a single descriptor table.
		DX12Descriptor blockStart = m_Device->GetSRVDescriptorHeap(m_FrameId).GetUserDescriptorBlockStart(1);
		m_Device->CopyDescriptorsSimple(1, blockStart.cpuHandle, srv.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_GraphicsCommandList->SetDescriptorTable(blockStart.gpuHandle);
	}

	void DX12GraphicsContext::SetPushConstants(const void* data, const uint32 size)
	{
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
		VAST_PROFILE_SCOPE("gfx", "Draw Instanced");
		m_GraphicsCommandList->GetCommandList()->DrawInstanced(vtxCountPerInstance, instCount, vtxStartLocation, instStartLocation);
	}

	void DX12GraphicsContext::DrawIndexedInstanced(uint32 idxCountPerInst, uint32 instCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation)
	{
		VAST_PROFILE_SCOPE("gfx", "Draw Indexed Instanced");
		m_GraphicsCommandList->GetCommandList()->DrawIndexedInstanced(idxCountPerInst, instCount, startIdxLocation, baseVtxLocation, startInstLocation);
	}

	void DX12GraphicsContext::DrawFullscreenTriangle()
	{
		VAST_PROFILE_SCOPE("gfx", "Draw Fullscreen Triangle");
		m_GraphicsCommandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		m_GraphicsCommandList->GetCommandList()->IASetIndexBuffer(nullptr);
		DrawInstanced(3, 1);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Resource Creation/Destruction/Update
	///////////////////////////////////////////////////////////////////////////////////////////////

	BufferHandle DX12GraphicsContext::CreateBuffer(const BufferDesc& desc, void* initialData /*= nullptr*/, const size_t dataSize /*= 0*/)
	{
		VAST_PROFILE_SCOPE("gfx", "Create Buffer");
		VAST_ASSERT(m_Device);
		auto [h, buf] = m_Buffers->AcquireResource();
		m_Device->CreateBuffer(desc, buf);
		buf->SetName("Unnamed Buffer");
		if (initialData != nullptr)
		{
			UpdateBuffer_Internal(buf, initialData, dataSize);
		}
		return h;
	}

	void DX12GraphicsContext::UpdateBuffer(const BufferHandle h, void* srcMem, const size_t srcSize)
	{
		VAST_PROFILE_SCOPE("gfx", "Update Buffer");
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		// TODO: Check buffer does not have UAV
		// TODO: Offer option to use buffered approach to improve performance (how do we handle updating descriptors?)
		UpdateBuffer_Internal(buf, srcMem, srcSize);
	}

	void DX12GraphicsContext::UpdateBuffer_Internal(DX12Buffer* buf, void* srcMem, size_t srcSize)
	{
		VAST_PROFILE_SCOPE("gfx", "Update Buffer");
		VAST_ASSERT(buf && srcMem && srcSize);

		switch (buf->usage)
		{
		case ResourceUsage::DEFAULT:
		{
			Ptr<BufferUpload> upload = MakePtr<BufferUpload>();
			upload->buf = buf;
			upload->data = MakePtr<uint8[]>(srcSize);
			upload->size = srcSize;
			memcpy(upload->data.get(), srcMem, srcSize);
			m_UploadCommandLists[m_FrameId]->UploadBuffer(std::move(upload));
			break;
		}
		case ResourceUsage::UPLOAD:
		{
			const auto dstSize = buf->resource->GetDesc().Width;
			VAST_ASSERT(srcSize > 0 && srcSize <= dstSize);
			uint8* dstMem = buf->data;
			memcpy(dstMem, srcMem, srcSize);
			break;
		}
		case ResourceUsage::READBACK:
		default:
			VAST_ASSERTF(0, "This path is not currently supported.");
			break;
		}
	}

	TextureHandle DX12GraphicsContext::CreateTexture(const TextureDesc& desc, void* initialData /*= nullptr*/)
	{
		VAST_PROFILE_SCOPE("gfx", "Create Texture");
		VAST_ASSERT(m_Device);
		auto [h, tex] = m_Textures->AcquireResource();
		m_Device->CreateTexture(desc, tex);
		tex->SetName("Unnamed Texture");
		if (initialData != nullptr)
		{
			UpdateTexture_Internal(tex, initialData);
		}
		return h;
	}

	void DX12GraphicsContext::UpdateTexture_Internal(DX12Texture* tex, void* srcMem)
	{
		VAST_PROFILE_SCOPE("gfx", "Update Texture");
		VAST_ASSERT(tex && srcMem);

		D3D12_RESOURCE_DESC desc = tex->resource->GetDesc();
		auto upload = std::make_unique<TextureUpload>();
		upload->tex = tex;
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

	static bool FileExists(const std::wstring& filePath)
	{
		if (filePath.c_str() == NULL)
			return false;

		DWORD fileAttr = GetFileAttributesW(filePath.c_str());
		if (fileAttr == INVALID_FILE_ATTRIBUTES)
			return false;

		return true;
	}

	static std::wstring GetFileExtension(const std::wstring& filePath)
	{
		size_t idx = filePath.rfind(L'.');
		if (idx != std::wstring::npos)
			return filePath.substr(idx + 1, filePath.length() - idx - 2);
		else
			return std::wstring(L"");
	}

	// From: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
	static std::wstring StringToWString(const std::string& s, bool bIsUTF8 = true)
	{
		int32 slength = (int32)s.length() + 1;
		int32 len = MultiByteToWideChar(bIsUTF8 ? CP_UTF8 : CP_ACP, 0, s.c_str(), slength, 0, 0);
		std::wstring buf;
		buf.resize(len);
		MultiByteToWideChar(bIsUTF8 ? CP_UTF8 : CP_ACP, 0, s.c_str(), slength, const_cast<wchar_t*>(buf.c_str()), len);
		return buf;
	}

	TextureHandle DX12GraphicsContext::CreateTexture(const std::string& filePath, bool sRGB /* = true */)
	{
		const std::wstring wpath = ASSETS_TEXTURES_PATH + StringToWString(filePath);
		if (!FileExists(wpath))
		{
			VAST_ASSERTF(0, "Texture file path does not exist.");
			return TextureHandle();
		}

		VAST_PROFILE_BEGIN("gfx", "Load Texture File");
		const std::wstring wext = GetFileExtension(wpath);
		DirectX::ScratchImage image;
		if (wext.compare(L"DDS") == 0 || wext.compare(L"dds") == 0)
		{
			DX12Check(DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image));
		}
		else if (wext.compare(L"TGA") == 0 || wext.compare(L"tga") == 0)
		{
			DirectX::ScratchImage tempImage;
			DX12Check(DirectX::LoadFromTGAFile(wpath.c_str(), nullptr, tempImage));
			DX12Check(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
		}
		else // Windows Imaging Component (WIC) includes .BMP, .PNG, .GIF, .TIFF, .JPEG
		{
			DirectX::ScratchImage tempImage;
			DX12Check(DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, tempImage));
			DX12Check(DirectX::GenerateMipMaps(*tempImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, image, false));
		}
		VAST_PROFILE_END("gfx", "Load Texture File");

		const DirectX::TexMetadata& metaData = image.GetMetadata();
		DXGI_FORMAT format = metaData.format;
		if (sRGB)
		{
			format = DirectX::MakeSRGB(format);
		}

		TexType type = TranslateFromDX12(static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension));

		TextureDesc texDesc =
		{
			.type = type,
			.format = TranslateFromDX12(metaData.format),
			.width  = static_cast<uint32>(metaData.width),
			.height = static_cast<uint32>(metaData.height),
			.depthOrArraySize = static_cast<uint32>((type == TexType::TEXTURE_3D) ? metaData.depth : metaData.arraySize),
			.mipCount = static_cast<uint32>(metaData.mipLevels),
			.viewFlags = TexViewFlags::SRV, // TODO: Provide option to add more flags when needed
		};
		return CreateTexture(texDesc, image.GetPixels());
	}

	PipelineHandle DX12GraphicsContext::CreatePipeline(const PipelineDesc& desc)
	{
		VAST_PROFILE_SCOPE("gfx", "Create Pipeline");
		VAST_ASSERT(m_Device);
		auto [h, pipeline] = m_Pipelines->AcquireResource();
		m_Device->CreatePipeline(desc, pipeline);
		return h;
	}

	void DX12GraphicsContext::UpdatePipeline(const PipelineHandle h)
	{
		VAST_PROFILE_SCOPE("gfx", "Update Pipeline");
		VAST_ASSERT(h.IsValid());
		auto pipeline = m_Pipelines->LookupResource(h);

		WaitForIdle(); // TODO TEMP: We should cache pipelines that want to update on a given frame and reload them all at a safe place, but this does the job for now.

		m_Device->UpdatePipeline(pipeline);
	}

	void DX12GraphicsContext::DestroyTexture(const TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_TexturesMarkedForDestruction[m_FrameId].push_back(h);
	}

	void DX12GraphicsContext::DestroyBuffer(const BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_BuffersMarkedForDestruction[m_FrameId].push_back(h);
	}

	void DX12GraphicsContext::DestroyPipeline(const PipelineHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_PipelinesMarkedForDestruction[m_FrameId].push_back(h);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	ShaderResourceProxy DX12GraphicsContext::LookupShaderResource(const PipelineHandle h, const std::string& shaderResourceName)
	{
		VAST_ASSERT(h.IsValid());
		auto pipeline = m_Pipelines->LookupResource(h);
		if (pipeline && pipeline->resourceProxyTable->IsRegistered(shaderResourceName))
		{
			return pipeline->resourceProxyTable->LookupShaderResource(shaderResourceName);
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

	TexFormat DX12GraphicsContext::GetTextureFormat(const TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		// Note: Clear Value stores the format given on resource creation, while the format stored
		// in the descriptor is modified for depth/stencil targets to store a TYPELESS equivalent.
		// We could translate back to the original format, but since we have this...
		return TranslateFromDX12(m_Textures->LookupResource(h)->clearValue.Format);
	}

	uint32 DX12GraphicsContext::GetBindlessIndex(const BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf && buf->descriptorHeapIdx != kInvalidHeapIdx);
		return buf->descriptorHeapIdx;
	}

	uint32 DX12GraphicsContext::GetBindlessIndex(const TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		auto tex = m_Textures->LookupResource(h);
		VAST_ASSERT(tex && tex->descriptorHeapIdx != kInvalidHeapIdx);
		return tex->descriptorHeapIdx;
	}

	bool DX12GraphicsContext::GetIsReady(const BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Buffers->LookupResource(h)->isReady;
	}

	bool DX12GraphicsContext::GetIsReady(const TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return m_Textures->LookupResource(h)->isReady;
	}

	void DX12GraphicsContext::SetDebugName(BufferHandle h, const std::string& name)
	{
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		buf->SetName(name);
	}

	void DX12GraphicsContext::SetDebugName(TextureHandle h, const std::string& name)
	{
		VAST_ASSERT(h.IsValid());
		auto tex = m_Textures->LookupResource(h);
		VAST_ASSERT(tex);
		tex->SetName(name);
	}

	void DX12GraphicsContext::ProcessDestructions(uint32 frameId)
	{
		VAST_PROFILE_SCOPE("gfx", "Process Destructions");
		VAST_ASSERT(m_Device);

		for (auto& h : m_BuffersMarkedForDestruction[frameId])
		{
			DX12Buffer* buf = m_Buffers->LookupResource(h);
			VAST_ASSERT(buf);
			m_Device->DestroyBuffer(buf);
			buf->Reset();
			m_Buffers->FreeResource(h);
		}
		m_BuffersMarkedForDestruction[frameId].clear();

		for (auto& h : m_TexturesMarkedForDestruction[frameId])
		{
			DX12Texture* tex = m_Textures->LookupResource(h);
			VAST_ASSERT(tex);
			m_Device->DestroyTexture(tex);
			tex->Reset();
			m_Textures->FreeResource(h);
		}
		m_TexturesMarkedForDestruction[frameId].clear();

		for (auto& h : m_PipelinesMarkedForDestruction[frameId])
		{
			DX12Pipeline* pipeline = m_Pipelines->LookupResource(h);
			VAST_ASSERT(pipeline);
			m_Device->DestroyPipeline(pipeline);
			pipeline->Reset();
			m_Pipelines->FreeResource(h);
		}
		m_PipelinesMarkedForDestruction[frameId].clear();
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

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Temp Frame Allocator
	///////////////////////////////////////////////////////////////////////////////////////////////

	BufferView DX12GraphicsContext::AllocTempBufferView(uint32 size, uint32 alignment /* = 0 */)
	{
		VAST_ASSERT(size);
		VAST_ASSERT(m_TempFrameAllocators[m_FrameId].size);

		uint32 allocSize = size + alignment;
		uint32 offset = m_TempFrameAllocators[m_FrameId].offset;
		if (alignment > 0)
		{
			offset = AlignU32(offset, alignment);
		}
		VAST_ASSERT(offset + size <= m_TempFrameAllocators[m_FrameId].size);

		m_TempFrameAllocators[m_FrameId].offset += allocSize;

		auto buf = m_Buffers->LookupResource(m_TempFrameAllocators[m_FrameId].buffer);
		VAST_ASSERT(buf);

		return BufferView
		{
			// TODO: If we separate handle from data in HandlePool, each BufferView could have its own handle, and we could even generalize BufferViews to just Buffer objects.
			m_TempFrameAllocators[m_FrameId].buffer,
			(uint8*)(buf->data + offset),
			offset,
		};
	}

	void DX12GraphicsContext::CreateTempFrameAllocators()
	{
		VAST_PROFILE_SCOPE("gfx", "Create Temp Frame Allocators");
		uint32 tempFrameBufferSize = 1024 * 1024;

		BufferDesc tempFrameBufferDesc =
		{
			.size = tempFrameBufferSize * 2, // TODO: Alignment?
			.usage = ResourceUsage::UPLOAD,
			.isRawAccess = true,
		};

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			VAST_ASSERT(!m_TempFrameAllocators[i].buffer.IsValid());
			m_TempFrameAllocators[i].buffer = CreateBuffer(tempFrameBufferDesc);
			m_TempFrameAllocators[i].size = tempFrameBufferSize;
			m_TempFrameAllocators[i].offset = 0;
		}
	}

	void DX12GraphicsContext::DestroyTempFrameAllocators()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DestroyBuffer(m_TempFrameAllocators[i].buffer);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
}