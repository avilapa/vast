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

	static bool s_bHasBackBufferBeenCleared = false;
	static bool s_bIsInRenderPass = false;

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
		, m_OutputRTHandle()
		, m_OutputRT(nullptr)
		, m_OutputToBackBufferPSO(nullptr)
		, m_TempFrameAllocators({})
		, m_FrameId(0)
	{
		VAST_PROFILE_FUNCTION();

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
		CreateOutputPassResources();

		VAST_SUBSCRIBE_TO_EVENT("gfxctx", WindowResizeEvent, VAST_EVENT_HANDLER_CB(DX12GraphicsContext::OnWindowResizeEvent, WindowResizeEvent));
	}

	DX12GraphicsContext::~DX12GraphicsContext()
	{
		VAST_PROFILE_FUNCTION();

		WaitForIdle();
		
		DestroyTempFrameAllocators();
		DestroyOutputPassResources();

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
		VAST_PROFILE_FUNCTION();
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
		VAST_PROFILE_FUNCTION();

		m_UploadCommandLists[m_FrameId]->ProcessUploads();
		SubmitCommandList(*m_UploadCommandLists[m_FrameId]);
		SignalEndOfFrame(QueueType::UPLOAD);

		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		m_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
		m_GraphicsCommandList->FlushBarriers();

		SubmitCommandList(*m_GraphicsCommandList);
		m_SwapChain->Present();
		s_bHasBackBufferBeenCleared = false;
		SignalEndOfFrame(QueueType::GRAPHICS);
	}

	void DX12GraphicsContext::FlushGPU()
	{
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
			m_CommandQueues[IDX(QueueType::GRAPHICS)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
		// TODO: Compute
		case D3D12_COMMAND_LIST_TYPE_COPY:
			m_CommandQueues[IDX(QueueType::UPLOAD)]->ExecuteCommandList(cmdList.GetCommandList());
			break;
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
		VAST_PROFILE_FUNCTION();
		for (auto& q : m_CommandQueues)
		{
			q->WaitForIdle();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Render Passes
	///////////////////////////////////////////////////////////////////////////////////////////////

	DX12RenderPassData DX12GraphicsContext::SetupBackBufferRenderPassBarrierTransitions()
	{
		DX12Texture& backBuffer = m_SwapChain->GetCurrentBackBuffer();
		m_GraphicsCommandList->AddBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// No need for an end barrier since we only present once at the end of the frame.
		DX12RenderPassData rdp;
		rdp.rtCount = 1;
		rdp.rtDesc[0].cpuDescriptor = backBuffer.rtv.cpuHandle;
		if (s_bHasBackBufferBeenCleared)
		{
			rdp.rtDesc[0].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
		}
		else
		{
			rdp.rtDesc[0].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			s_bHasBackBufferBeenCleared = true;
			// TODO: Clear Value
		}
		rdp.rtDesc[0].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

		return rdp;
	}

	DX12RenderPassData DX12GraphicsContext::SetupCommonRenderPassBarrierTransitions(DX12Pipeline* pipeline, RenderPassTargets targets)
	{
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
			// TODO: Fill Stencil access
		}
		return rpd;
	}

	void DX12GraphicsContext::BeginRenderPass_Internal(const DX12RenderPassData& rpd)
	{
		m_GraphicsCommandList->FlushBarriers();

		VAST_ASSERTF(!s_bIsInRenderPass, "BeginRenderPass called before previous EndRenderPass call.");
		m_GraphicsCommandList->BeginRenderPass(rpd);
		s_bIsInRenderPass = true;

		m_GraphicsCommandList->SetDefaultViewportAndScissor(m_SwapChain->GetSize()); // TODO: This shouldn't be here!
	}

	void DX12GraphicsContext::BeginRenderPassToBackBuffer_Internal(DX12Pipeline* pipeline)
	{
		VAST_PROFILE_SCOPE("DX12GraphicsContext", "BeginRenderPassToBackBuffer");

		VAST_ASSERT(pipeline);
		m_GraphicsCommandList->SetPipeline(pipeline);

		DX12RenderPassData rdp = SetupBackBufferRenderPassBarrierTransitions();
		BeginRenderPass_Internal(rdp);
	}

	void DX12GraphicsContext::BeginRenderPassToBackBuffer(const PipelineHandle h)
	{
		VAST_ASSERT(m_Pipelines && h.IsValid());
		BeginRenderPassToBackBuffer_Internal(m_Pipelines->LookupResource(h));
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
		VAST_PROFILE_SCOPE("DX12GraphicsContext", "BeginRenderPass");
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
		VAST_PROFILE_SCOPE("DX12GraphicsContext", "EndRenderPass");

		VAST_ASSERTF(s_bIsInRenderPass, "EndRenderPass called without matching BeginRenderPass call.");
		m_GraphicsCommandList->EndRenderPass();
		s_bIsInRenderPass = false;

		for (auto i : m_RenderPassEndBarriers)
		{
			m_GraphicsCommandList->AddBarrier(*i.first, i.second);
		}
		m_RenderPassEndBarriers.clear();

		m_GraphicsCommandList->SetPipeline(nullptr);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Resource Binding
	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::SetVertexBuffer(const BufferHandle h, uint32 offset /* = 0 */, uint32 stride /* = 0 */)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetVertexBuffer(*buf, offset, stride);
	}

	void DX12GraphicsContext::SetIndexBuffer(const BufferHandle h, uint32 offset /* = 0 */, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetIndexBuffer(*buf, offset, TranslateToDX12(format));
	}

	void DX12GraphicsContext::SetShaderResource(const BufferHandle h, const ShaderResourceProxy shaderResourceProxy)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(shaderResourceProxy.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		VAST_ASSERT(buf);
		m_GraphicsCommandList->SetConstantBuffer(*buf, 0, shaderResourceProxy.idx);
	}

	void DX12GraphicsContext::SetShaderResource(const TextureHandle h, const ShaderResourceProxy shaderResourceProxy)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		VAST_ASSERT(shaderResourceProxy.IsValid());
		auto tex = m_Textures->LookupResource(h);
		VAST_ASSERT(tex);
		(void)shaderResourceProxy; // TODO: This is currently not being used as an index, only to check that the name exists in the shader.
		// TODO TEMP: We should accumulate all SRV/UAV per shader space and combine them into a single descriptor table.
		DX12Descriptor blockStart = m_Device->GetSRVDescriptorHeap(m_FrameId).GetUserDescriptorBlockStart(1);
		m_Device->CopyDescriptorsSimple(1, blockStart.cpuHandle, tex->srv.cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_GraphicsCommandList->SetDescriptorTable(blockStart.gpuHandle);
	}

	void DX12GraphicsContext::SetPushConstants(const void* data, const uint32 size)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(data && size);
		m_GraphicsCommandList->SetPushConstants(data, size);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::SetScissorRect(int4 rect)
	{
		VAST_PROFILE_FUNCTION();
		const D3D12_RECT r = { (LONG)rect.x, (LONG)rect.y, (LONG)rect.z, (LONG)rect.w };
		m_GraphicsCommandList->SetScissorRect(r);
	}

	void DX12GraphicsContext::SetBlendFactor(float4 blend)
	{
		VAST_PROFILE_FUNCTION();
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
		VAST_PROFILE_FUNCTION();
		m_GraphicsCommandList->GetCommandList()->DrawInstanced(vtxCountPerInstance, instCount, vtxStartLocation, instStartLocation);
	}

	void DX12GraphicsContext::DrawIndexedInstanced(uint32 idxCountPerInst, uint32 instCount, uint32 startIdxLocation, uint32 baseVtxLocation, uint32 startInstLocation)
	{
		VAST_PROFILE_FUNCTION();
		m_GraphicsCommandList->GetCommandList()->DrawIndexedInstanced(idxCountPerInst, instCount, startIdxLocation, baseVtxLocation, startInstLocation);
	}

	void DX12GraphicsContext::DrawFullscreenTriangle()
	{
		VAST_PROFILE_FUNCTION();
		m_GraphicsCommandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		m_GraphicsCommandList->GetCommandList()->IASetIndexBuffer(nullptr);
		DrawInstanced(3, 1);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Resource Creation/Destruction/Update
	///////////////////////////////////////////////////////////////////////////////////////////////

	BufferHandle DX12GraphicsContext::CreateBuffer(const BufferDesc& desc, void* initialData /*= nullptr*/, const size_t dataSize /*= 0*/)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(m_Device);
		auto [h, buf] = m_Buffers->AcquireResource();
		m_Device->CreateBuffer(desc, buf);
		if (initialData != nullptr)
		{
			if (desc.cpuAccess == BufCpuAccess::WRITE)
			{
				SetBufferData(buf, initialData, dataSize);
			}
			else
			{
				UploadBufferData(buf, initialData, dataSize);
			}
		}
		return h;
	}

	void DX12GraphicsContext::UpdateBuffer(const BufferHandle h, void* srcMem, const size_t srcSize)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		auto buf = m_Buffers->LookupResource(h);
		// TODO: Check buffer does not have UAV
		VAST_ASSERTF(buf->usage == ResourceUsage::DYNAMIC, "Attempted to update non dynamic resource.");
		// TODO: Dynamic no CPU access buffers
		// TODO: Use buffered approach to improve performance (how do we handle updating descriptors?)
		SetBufferData(buf, srcMem, srcSize);
	}
	
	void DX12GraphicsContext::UpdatePipeline(const PipelineHandle h)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(h.IsValid());
		auto pipeline = m_Pipelines->LookupResource(h);

		WaitForIdle(); // TODO TEMP: We should cache pipelines that want to update on a given frame and reload them all at a safe place, but this does the job for now.

		m_Device->UpdatePipeline(pipeline);
	}

	void DX12GraphicsContext::SetBufferData(DX12Buffer* buf, void* srcMem, size_t srcSize)
	{
		VAST_ASSERT(buf);
		VAST_ASSERT(srcMem && srcSize);
		const auto dstSize = buf->resource->GetDesc().Width;
		VAST_ASSERT(srcSize > 0 && srcSize <= dstSize);

		uint8* dstMem = buf->data;
		memcpy(dstMem, srcMem, srcSize);
	}

	void DX12GraphicsContext::UploadBufferData(DX12Buffer* buf, void* srcMem, size_t srcSize)
	{
		VAST_ASSERT(buf);
		VAST_ASSERT(srcMem && srcSize);

		Ptr<BufferUpload> upload = MakePtr<BufferUpload>();
		upload->buf = buf;
		upload->data = MakePtr<uint8[]>(srcSize);
		upload->size = srcSize;

		memcpy(upload->data.get(), srcMem, srcSize);
		m_UploadCommandLists[m_FrameId]->UploadBuffer(std::move(upload));
	}

	TextureHandle DX12GraphicsContext::CreateTexture(const TextureDesc& desc, void* initialData /*= nullptr*/)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(m_Device);
		auto [h, tex] = m_Textures->AcquireResource();
		m_Device->CreateTexture(desc, tex);
		if (initialData != nullptr)
		{
			UploadTextureData(tex, initialData);
		}
		return h;
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

	TextureHandle DX12GraphicsContext::CreateTexture(const std::string& file, bool sRGB /* = true */)
	{
		VAST_PROFILE_FUNCTION();
		const std::wstring wpath = ASSETS_TEXTURES_PATH + StringToWString(file);

		if (!FileExists(wpath))
		{
			VAST_ASSERTF(0, "Texture file path does not exist.");
			return TextureHandle();
		}

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

	void DX12GraphicsContext::UploadTextureData(DX12Texture* tex, void* srcMem)
	{
		VAST_ASSERT(tex);
		VAST_ASSERT(srcMem);

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

	PipelineHandle DX12GraphicsContext::CreatePipeline(const PipelineDesc& desc)
	{
		VAST_PROFILE_FUNCTION();
		VAST_ASSERT(m_Device);
		auto [h, pipeline] = m_Pipelines->AcquireResource();
		m_Device->CreatePipeline(desc, pipeline);
		return h;
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
	
	TexFormat DX12GraphicsContext::GetTextureFormat(const TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return TranslateFromDX12(m_Textures->LookupResource(h)->resource->GetDesc().Format);
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

	void DX12GraphicsContext::ProcessDestructions(uint32 frameId)
	{
		VAST_PROFILE_FUNCTION();
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
		VAST_PROFILE_FUNCTION();

		const uint2 scSize = m_SwapChain->GetSize();

		if (event.m_WindowSize.x != scSize.x || event.m_WindowSize.y != scSize.y)
		{
			WaitForIdle();
			// TODO: Resize() here returns the BackBuffer index after resize, which is 0. We used
			// to assign this to m_FrameId as if resetting the count for these to be in sync, but
			// this appears to cause issues (would need to reset some buffered members), and also
			// it doesn't even make sense since they will lose sync after the first loop.
			m_SwapChain->Resize(event.m_WindowSize);
			ResizeOutputRenderTarget();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// OutputRT -> BackBuffer
	///////////////////////////////////////////////////////////////////////////////////////////////

	void DX12GraphicsContext::RenderOutputToBackBuffer()
	{
		VAST_PROFILE_SCOPE("DX12GraphicsContext", "RenderOutputToBackBuffer");
		BeginRenderPassToBackBuffer_Internal(m_OutputToBackBufferPSO.get());
		{
			VAST_ASSERT(m_OutputRT && m_OutputRT->descriptorHeapIdx != kInvalidHeapIdx);
			SetPushConstants(&m_OutputRT->descriptorHeapIdx, sizeof(uint32));
			DrawFullscreenTriangle();
		}
		EndRenderPass();
	}

	TextureHandle DX12GraphicsContext::GetOutputRenderTarget() const
	{
		VAST_ASSERT(m_OutputRTHandle.IsValid());
		return m_OutputRTHandle;
	}

	TexFormat DX12GraphicsContext::GetOutputRenderTargetFormat() const
	{
		VAST_ASSERT(m_OutputRT && m_OutputRT->resource);
		TexFormat format = TranslateFromDX12(m_OutputRT->resource->GetDesc().Format);
		VAST_ASSERT(m_SwapChain && m_SwapChain->GetFormat() == format);
		return format;
	}

	uint2 DX12GraphicsContext::GetOutputRenderTargetSize() const
	{
		VAST_ASSERT(m_OutputRT && m_OutputRT->resource);
		uint2 size = uint2(m_OutputRT->resource->GetDesc().Width, m_OutputRT->resource->GetDesc().Height);
		VAST_ASSERT(m_SwapChain && m_SwapChain->GetSize().x == size.x && m_SwapChain->GetSize().y == size.y);
		return size;
	}

	void DX12GraphicsContext::CreateOutputPassResources()
	{
		VAST_ASSERT(!m_OutputToBackBufferPSO);
		m_OutputToBackBufferPSO = MakePtr<DX12Pipeline>();
		m_Device->CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "fullscreen.hlsl", .entryPoint = "VS_Main"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "fullscreen.hlsl", .entryPoint = "PS_Main"},
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { m_SwapChain->GetFormat() } },
		}, m_OutputToBackBufferPSO.get());

		VAST_ASSERT(!m_OutputRTHandle.IsValid() && !m_OutputRT);
		auto [h, tex] = m_Textures->AcquireResource();
		m_Device->CreateTexture(AllocRenderTargetDesc(m_SwapChain->GetBackBufferFormat(), m_SwapChain->GetSize()), tex);

		// We need the handle to allow the user to render to this target.
		m_OutputRTHandle = h;
		m_OutputRT = tex;
	}

	void DX12GraphicsContext::DestroyOutputPassResources()
	{
		VAST_ASSERT(m_OutputToBackBufferPSO);
		m_Device->DestroyPipeline(m_OutputToBackBufferPSO.get());
		m_OutputToBackBufferPSO->Reset();

		DestroyTexture(m_OutputRTHandle);
	}

	void DX12GraphicsContext::ResizeOutputRenderTarget()
	{
		// GPU must be flushed so that we can recreate the target in place.
		m_Device->DestroyTexture(m_OutputRT);
		m_OutputRT->Reset();
		m_Device->CreateTexture(AllocRenderTargetDesc(m_SwapChain->GetBackBufferFormat(), m_SwapChain->GetSize()), m_OutputRT);
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	// Temp Frame Allocator
	///////////////////////////////////////////////////////////////////////////////////////////////

	BufferView DX12GraphicsContext::AllocTempBufferView(uint32 size, uint32 alignment /* = 0 */)
	{
		VAST_PROFILE_FUNCTION();
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
		uint32 tempFrameBufferSize = 1024 * 1024;

		BufferDesc tempFrameBufferDesc =
		{
			.size = tempFrameBufferSize * 2, // TODO: Alignment?
			.cpuAccess = BufCpuAccess::WRITE,
			// TODO: Should this be dynamic?
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
		DestroyBuffer(m_TempFrameAllocators[0].buffer);
		DestroyBuffer(m_TempFrameAllocators[1].buffer);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
}