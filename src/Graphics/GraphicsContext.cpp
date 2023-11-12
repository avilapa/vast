#include "vastpch.h"
#include "Graphics/GraphicsContext.h"

#if VAST_GFX_DX12_SUPPORTED
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#else
#error "Invalid Platform: Unknown Graphics Backend"
#endif

#include "dx12/DirectXTex/DirectXTex/DirectXTex.h"
#include <iostream>

static const wchar_t* ASSETS_TEXTURES_PATH = L"../../assets/textures/";

namespace vast::gfx
{
	// TODO: Current goal with Graphics Context refactor is to see if we can make it agnostic enough that really all we need
	// is a RenderBackend and no virtual class at all for Graphics Context. Right now many of the calls would be straight
	// up pointless indirections, but with pipeline caching many of that may be bundled together in the Graphics Context before
	// reaching the device/cmdlist.
	Ptr<GraphicsContext> GraphicsContext::Create(const GraphicsParams& params /* = GraphicsParams() */)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Graphics Context");
		VAST_LOG_INFO("[gfx] Initializing GraphicsContext...");
#ifdef VAST_GFX_DX12_SUPPORTED
		return MakePtr<DX12GraphicsContext>(params);
#else
		VAST_ASSERTF(0, "Invalid Platform: Unknown Graphics Backend");
		return nullptr;
#endif
	}

	//

	void GraphicsContext::BeginFrame()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Begin Frame");
		VAST_ASSERTF(!m_bHasFrameBegun, "A frame is already running");
		m_bHasFrameBegun = true;

		if (!m_PipelinesMarkedForUpdate.empty())
		{
			// Pipelines are not double buffered, so we need a hard wait to reload shaders in place.
			WaitForIdle();
			UpdateMarkedPipelines();
		}

		m_FrameId = (m_FrameId + 1) % NUM_FRAMES_IN_FLIGHT;

		// TODO: Figure out where the profiling for destructions should go (cpu/gpu?)
		ProcessDestructions(m_FrameId);

		m_TempFrameAllocators[m_FrameId].Reset();

		BeginFrame_Internal();

		m_GpuFrameTimestampIdx = BeginTimestamp();
	}

	void GraphicsContext::EndFrame()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "End Frame");
		VAST_ASSERTF(m_bHasFrameBegun, "No frame is currently running.");

		EndTimestamp(m_GpuFrameTimestampIdx);
		CollectTimestamps();

		EndFrame_Internal();

		m_bHasFrameBegun = false;
	}

	bool GraphicsContext::IsInFrame() const
	{
		return m_bHasFrameBegun;
	}

	void GraphicsContext::FlushGPU()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Flush GPU Work");
		WaitForIdle();

		UpdateMarkedPipelines();

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			ProcessDestructions(i);
		}
	}

	//

	bool GraphicsContext::IsInRenderPass() const
	{
		return m_bHasRenderPassBegun;
	}

	//

	BufferHandle GraphicsContext::CreateBuffer(const BufferDesc& desc, const void* initialData /*= nullptr*/, const size_t dataSize /*= 0*/)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Buffer");
		BufferHandle h = m_BufferHandles->AllocHandle();
		CreateBuffer_Internal(h, desc);
		if (initialData != nullptr)
		{
			UpdateBuffer_Internal(h, initialData, dataSize);
		}
		return h;
	}

	TextureHandle GraphicsContext::CreateTexture(const TextureDesc& desc, const void* initialData /*= nullptr*/)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Texture");
		TextureHandle h = m_TextureHandles->AllocHandle();
		CreateTexture_Internal(h, desc);
		if (initialData != nullptr)
		{
			UpdateTexture_Internal(h, initialData);
		}
		return h;
	}

	PipelineHandle GraphicsContext::CreatePipeline(const PipelineDesc& desc)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Pipeline");
		PipelineHandle h = m_PipelineHandles->AllocHandle();
		CreatePipeline_Internal(h, desc);
		return h;
	}
	
	PipelineHandle GraphicsContext::CreatePipeline(const ShaderDesc& csDesc)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Pipeline");
		PipelineHandle h = m_PipelineHandles->AllocHandle();
		CreatePipeline_Internal(h, csDesc);
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

	// TODO: This should be completely external to the Graphics Context
	TextureHandle GraphicsContext::LoadTextureFromFile(const std::string& filePath, bool sRGB /* = true */)
	{
		bool bFlipImage = false;

		const std::wstring wpath = ASSETS_TEXTURES_PATH + StringToWString(filePath);
		if (!FileExists(wpath))
		{
			VAST_ASSERTF(0, "Texture file path does not exist.");
			return TextureHandle();
		}

		VAST_PROFILE_TRACE_BEGIN("gfx", "Load Texture File");
		const std::wstring wext = GetFileExtension(wpath);
		DirectX::ScratchImage image;
		if (wext.compare(L"DDS") == 0 || wext.compare(L"dds") == 0)
		{
			DX12Check(DirectX::LoadFromDDSFile(wpath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image));
		}
		else if (wext.compare(L"TGA") == 0 || wext.compare(L"tga") == 0)
		{
			DX12Check(DirectX::LoadFromTGAFile(wpath.c_str(), nullptr, image));
		}
		else if (wext.compare(L"HDR") == 0 || wext.compare(L"hdr") == 0)
		{
			DX12Check(DirectX::LoadFromHDRFile(wpath.c_str(), nullptr, image));
		}
		else // Windows Imaging Component (WIC) includes .BMP, .PNG, .GIF, .TIFF, .JPEG
		{
			DX12Check(DirectX::LoadFromWICFile(wpath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image));
		}
		VAST_PROFILE_TRACE_END("gfx", "Load Texture File");

		if (bFlipImage)
		{
			DirectX::ScratchImage tempImage;
			DX12Check(DirectX::FlipRotate(image.GetImages()[0], DirectX::TEX_FR_FLIP_VERTICAL, tempImage));
			image = std::move(tempImage);
		}

		if (image.GetMetadata().mipLevels == 1)
		{
			DirectX::ScratchImage tempImage;
			DX12Check(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, tempImage));
			image = std::move(tempImage);
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
			.width = static_cast<uint32>(metaData.width),
			.height = static_cast<uint32>(metaData.height),
			.depthOrArraySize = static_cast<uint32>((type == TexType::TEXTURE_3D) ? metaData.depth : metaData.arraySize),
			.mipCount = static_cast<uint32>(metaData.mipLevels),
			.viewFlags = TexViewFlags::SRV, // TODO: Provide option to add more flags when needed
			.name = filePath,
		};
		return CreateTexture(texDesc, std::move(image.GetPixels()));
	}

	void GraphicsContext::UpdateBuffer(BufferHandle h, void* data, const size_t size)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Update Buffer");
		VAST_ASSERT(h.IsValid());
		UpdateBuffer_Internal(h, data, size);
	}

	void GraphicsContext::UpdatePipeline(PipelineHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_PipelinesMarkedForUpdate.push_back(h);
	}

	void GraphicsContext::UpdateMarkedPipelines()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Update Cached Pipelines");
		for (auto& h : m_PipelinesMarkedForUpdate)
		{
			VAST_ASSERT(h.IsValid());
			UpdatePipeline_Internal(h);
		}
		m_PipelinesMarkedForUpdate.clear();
	}

	void GraphicsContext::DestroyBuffer(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_BuffersMarkedForDestruction[m_FrameId].push_back(h);
	}

	void GraphicsContext::DestroyTexture(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_TexturesMarkedForDestruction[m_FrameId].push_back(h);
	}

	void GraphicsContext::DestroyPipeline(PipelineHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_PipelinesMarkedForDestruction[m_FrameId].push_back(h);
	}

	void GraphicsContext::ProcessDestructions(uint32 frameId)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Process Destructions");

		for (auto& h : m_BuffersMarkedForDestruction[frameId])
		{
			VAST_ASSERT(h.IsValid());
			DestroyBuffer_Internal(h);
			m_BufferHandles->FreeHandle(h);
		}
		m_BuffersMarkedForDestruction[frameId].clear();

		for (auto& h : m_TexturesMarkedForDestruction[frameId])
		{
			VAST_ASSERT(h.IsValid());
			DestroyTexture_Internal(h);
			m_TextureHandles->FreeHandle(h);
		}
		m_TexturesMarkedForDestruction[frameId].clear();

		for (auto& h : m_PipelinesMarkedForDestruction[frameId])
		{
			VAST_ASSERT(h.IsValid());
			DestroyPipeline_Internal(h);
			m_PipelineHandles->FreeHandle(h);
		}
		m_PipelinesMarkedForDestruction[frameId].clear();
	}

	void GraphicsContext::CreateHandlePools()
	{
		m_BufferHandles = MakePtr<HandlePool<Buffer, NUM_BUFFERS>>();
		m_TextureHandles = MakePtr<HandlePool<Texture, NUM_TEXTURES>>();
		m_PipelineHandles = MakePtr<HandlePool<Pipeline, NUM_PIPELINES>>();
	}
	
	void GraphicsContext::DestroyHandlePools()
	{
		m_BufferHandles = nullptr;
		m_TextureHandles = nullptr;
		m_PipelineHandles = nullptr;
	}

	void GraphicsContext::CreateTempFrameAllocators()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Temp Frame Allocators");
		uint32 tempFrameBufferSize = 1024 * 1024;

		BufferDesc tempFrameBufferDesc =
		{
			.size = tempFrameBufferSize,
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

	void GraphicsContext::DestroyTempFrameAllocators()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DestroyBuffer(m_TempFrameAllocators[i].buffer);
		}
	}

	BufferView GraphicsContext::AllocTempBufferView(uint32 size, uint32 alignment /* = 0 */)
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

		BufferHandle h = m_TempFrameAllocators[m_FrameId].buffer;

		// TODO: We need to attach descriptors to the BufferViews if we want to use them as CBVs, SRVs...
		return BufferView
		{
			// TODO: If we separate handle from data in HandlePool, each BufferView could have its own handle, and we could even generalize BufferViews to just Buffer objects.
			h,
			(uint8*)(GetBufferData(h) + offset),
			offset,
		};
	}

	//

	void GraphicsContext::CreateProfilingResources()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_TimestampsReadbackBuf[i] = CreateBuffer(BufferDesc{ .size = NUM_TIMESTAMP_QUERIES * sizeof(uint64),.usage = ResourceUsage::READBACK });
			m_TimestampData[i] = reinterpret_cast<const uint64*>(GetBufferData(m_TimestampsReadbackBuf[i]));
		}
	}
	
	void GraphicsContext::DestroyProfilingResources()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DestroyBuffer(m_TimestampsReadbackBuf[i]);
		}
	}

	uint32 GraphicsContext::BeginTimestamp()
	{
		BeginTimestamp_Internal(m_TimestampCount * 2);
		return m_TimestampCount++;
	}

	void GraphicsContext::EndTimestamp(uint32 timestampIdx)
	{
		EndTimestamp_Internal(timestampIdx * 2 + 1);
	}

	void GraphicsContext::CollectTimestamps()
	{
		if (m_TimestampCount == 0)
			return;

		CollectTimestamps_Internal(m_TimestampsReadbackBuf[m_FrameId], m_TimestampCount * 2);
		m_TimestampCount = 0;
	}

	double GraphicsContext::GetTimestampDuration(uint32 timestampIdx)
	{
		const uint64* data = m_TimestampData[m_FrameId];
		VAST_ASSERT(data && m_TimestampFrequency);

		uint64 tStart = data[timestampIdx * 2];
		uint64 tEnd = data[timestampIdx * 2 + 1];

		if (tEnd > tStart)
		{
			return double(tEnd - tStart) / m_TimestampFrequency;
		}

		return 0.0;
	}

	double GraphicsContext::GetLastFrameDuration()
	{
		return GetTimestampDuration(m_GpuFrameTimestampIdx);
	}

}
