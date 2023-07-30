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
		VAST_PROFILE_SCOPE("gfx", "Create Graphics Context");
		VAST_INFO("[gfx] Creating graphics context.");
#ifdef VAST_GFX_DX12_SUPPORTED
		return MakePtr<DX12GraphicsContext>(params);
#else
		VAST_ASSERTF(0, "Invalid Platform: Unknown Graphics Backend");
		return nullptr;
#endif
	}

	//

	bool GraphicsContext::IsInFrame() const
	{
		return m_bHasFrameBegun;
	}

	bool GraphicsContext::IsInRenderPass() const
	{
		return m_bHasRenderPassBegun;
	}

	//

	BufferHandle GraphicsContext::CreateBuffer(const BufferDesc& desc, void* initialData /*= nullptr*/, const size_t dataSize /*= 0*/)
	{
		VAST_PROFILE_SCOPE("gfx", "Create Buffer");
		BufferHandle h = m_BufferHandles->AllocHandle();
		CreateBuffer_Internal(h, desc);
		if (initialData != nullptr)
		{
			UpdateBuffer_Internal(h, initialData, dataSize);
		}
		return h;
	}

	TextureHandle GraphicsContext::CreateTexture(const TextureDesc& desc, void* initialData /*= nullptr*/)
	{
		VAST_PROFILE_SCOPE("gfx", "Create Texture");
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
		VAST_PROFILE_SCOPE("gfx", "Create Pipeline");
		PipelineHandle h = m_PipelineHandles->AllocHandle();
		CreatePipeline_Internal(h, desc);
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
			.width = static_cast<uint32>(metaData.width),
			.height = static_cast<uint32>(metaData.height),
			.depthOrArraySize = static_cast<uint32>((type == TexType::TEXTURE_3D) ? metaData.depth : metaData.arraySize),
			.mipCount = static_cast<uint32>(metaData.mipLevels),
			.viewFlags = TexViewFlags::SRV, // TODO: Provide option to add more flags when needed
		};
		return CreateTexture(texDesc, image.GetPixels());
	}

	void GraphicsContext::UpdateBuffer(BufferHandle h, void* data, const size_t size)
	{
		VAST_PROFILE_SCOPE("gfx", "Update Buffer");
		VAST_ASSERT(h.IsValid());
		UpdateBuffer_Internal(h, data, size);
	}

	void GraphicsContext::UpdatePipeline(PipelineHandle h)
	{
		VAST_PROFILE_SCOPE("gfx", "Update Pipeline");
		VAST_ASSERT(h.IsValid());
		UpdatePipeline_Internal(h);
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
		VAST_PROFILE_SCOPE("gfx", "Process Destructions");

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
		VAST_PROFILE_SCOPE("gfx", "Create Temp Frame Allocators");
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

	//

	void GraphicsContext::PushProfilingMarker(const std::string& name)
	{
		// TODO: Currently stacking timings is not supported.
		VAST_ASSERT(m_ActiveProfilingEntry == kInvalidProfilingEntryIdx);

		// Check if profile already exists
		for (uint32 i = 0; i < m_ProfilingEntryCount; ++i)
		{
			if (m_ProfilingEntries[i].name == name)
			{
				m_ActiveProfilingEntry = i;
				break;
			}
		}
		
		if (m_ActiveProfilingEntry == kInvalidProfilingEntryIdx)
		{
			// Add new profile
			VAST_ASSERTF(m_ProfilingEntryCount < kMaxProfilingEntries, "Exceeded max number of unique profiling markers ({}).", kMaxProfilingEntries);
			m_ActiveProfilingEntry = m_ProfilingEntryCount++;
			m_ProfilingEntries[m_ActiveProfilingEntry].name = name;
		}
		
		VAST_ASSERTF(m_ProfilingEntries[m_ActiveProfilingEntry].state == ProfilingEntryState::IDLE, "Profiling marker '{}' already pushed this frame.", name);
		m_ProfilingEntries[m_ActiveProfilingEntry].state = ProfilingEntryState::ACTIVE;

		// Insert timestamp
		BeginTiming_Internal(m_ActiveProfilingEntry * 2);
	}

	void GraphicsContext::PopProfilingMarker()
	{
		// TODO: Currently stacking timings is not supported.
		VAST_ASSERT(m_ActiveProfilingEntry != kInvalidProfilingEntryIdx);

		VAST_ASSERT(m_ProfilingEntries[m_ActiveProfilingEntry].state == ProfilingEntryState::ACTIVE);

		// Insert timestamp
		EndTiming_Internal(m_ActiveProfilingEntry * 2 + 1);
		
		m_ProfilingEntries[m_ActiveProfilingEntry].state = ProfilingEntryState::FINISHED;
		m_ActiveProfilingEntry = kInvalidProfilingEntryIdx;
	}

	void GraphicsContext::ReadProfilingData()
	{
		VAST_ASSERT(m_ActiveProfilingEntry == kInvalidProfilingEntryIdx);

		if (m_ProfilingEntryCount == 0)
			return;

		const uint64* queryData = ReadTimings_Internal(m_ProfileReadbackBuf[m_FrameId], m_ProfilingEntryCount * 2);
		const double gpuFrequency = static_cast<double>(GetTimestampFrequency());

		for (uint32 i = 0; i < m_ProfilingEntryCount; ++i)
		{
			double time = 0;

			uint64 startTime = queryData[i * 2];
			uint64 endTime = queryData[i * 2 + 1];

			if (endTime > startTime)
			{
				uint64 delta = endTime - startTime;
				time = (delta / gpuFrequency) * 1000.0;
			}

			ProfilingEntry& e = m_ProfilingEntries[i];
			e.deltasHistory[e.currDelta] = time;
			e.currDelta = (e.currDelta + 1) & ProfilingEntry::kHistorySize;
			e.state = ProfilingEntryState::IDLE;

			// TODO: We could compute min, max, avg, etc from the history
			VAST_INFO("'{}' Time: {} ms", e.name, time);
		}
	}

	void GraphicsContext::CreateProfilingResources()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			m_ProfileReadbackBuf[i] = CreateBuffer(BufferDesc{ .size = (kMaxProfilingEntries * 2) * sizeof(uint64),.usage = ResourceUsage::READBACK });
		}
	}
	
	void GraphicsContext::DestroyProfilingResources()
	{
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DestroyBuffer(m_ProfileReadbackBuf[i]);
		}
	}

}
