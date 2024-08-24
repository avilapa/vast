#include "vastpch.h"
#include "Graphics/GPUResourceManager.h"
#include "Graphics/GraphicsBackend.h"

// TODO: Hide DX12 specific headers (LoadTextureFromFile)
#include "Graphics/API/DX12/DX12_Common.h"
#include "dx12/DirectXTex/DirectXTex/DirectXTex.h"
#include <iostream>

static const wchar_t* ASSETS_TEXTURES_PATH = L"../../assets/textures/";

namespace vast
{

	GPUResourceManager::GPUResourceManager()
		: m_BufferHandles()
		, m_TextureHandles()
		, m_PipelineHandles()
		, m_BuffersMarkedForDestruction({})
		, m_TexturesMarkedForDestruction({})
		, m_PipelinesMarkedForDestruction({})
		, m_PipelinesMarkedForShaderReload({})
		, m_TempFrameAllocators({})
	{

		// Create frame allocators
		BufferDesc tempFrameBufferDesc =
		{
			.size = 1024 * 1024,
			.usage = ResourceUsage::UPLOAD,
			.isRawAccess = true,
		};

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			auto& frameAllocator = m_TempFrameAllocators[i];
			VAST_ASSERT(!frameAllocator.buffer.IsValid());
			frameAllocator.buffer = CreateBuffer(tempFrameBufferDesc);
			frameAllocator.bufferSize = tempFrameBufferDesc.size;
			frameAllocator.usedMemory = 0;
		}
	}

	GPUResourceManager::~GPUResourceManager()
	{
		gfx::WaitForIdle();

		// Destroy frame allocators
		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			DestroyBuffer(m_TempFrameAllocators[i].buffer);
		}

		for (uint32 i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
		{
			ProcessDestructions(i);
		}
	}

	void GPUResourceManager::BeginFrame()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		if (!m_PipelinesMarkedForShaderReload.empty())
		{
			// Pipelines are not double buffered, so we need a hard wait to reload shaders in place.
			gfx::WaitForIdle();
			ProcessShaderReloads();
		}

		uint32 frameId = gfx::GetFrameId();

		// TODO: Figure out where the profiling for destructions should go (cpu/gpu?)
		ProcessDestructions(frameId);

		// Invalidate frame allocator memory
		m_TempFrameAllocators[frameId].Reset();
	}

	BufferHandle GPUResourceManager::CreateBuffer(const BufferDesc& desc, const void* initialData /*= nullptr*/, const size_t dataSize /*= 0*/)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		BufferHandle h = m_BufferHandles.AllocHandle();
		gfx::CreateBuffer(h, desc);
		if (initialData != nullptr)
		{
			gfx::UpdateBuffer(h, initialData, dataSize);
		}
		return h;
	}

	TextureHandle GPUResourceManager::CreateTexture(const TextureDesc& desc, const void* initialData /*= nullptr*/)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		TextureHandle h = m_TextureHandles.AllocHandle();
		gfx::CreateTexture(h, desc);
		if (initialData != nullptr)
		{
			gfx::UpdateTexture(h, initialData);
		}
		return h;
	}

	PipelineHandle GPUResourceManager::CreatePipeline(const PipelineDesc& desc)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		PipelineHandle h = m_PipelineHandles.AllocHandle();
		gfx::CreatePipeline(h, desc);
		return h;
	}

	PipelineHandle GPUResourceManager::CreatePipeline(const ShaderDesc& csDesc)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		PipelineHandle h = m_PipelineHandles.AllocHandle();
		gfx::CreatePipeline(h, csDesc);
		return h;
	}

	// TODO: Move these to Filesystem
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
	TextureHandle GPUResourceManager::LoadTextureFromFile(const std::string& filePath, bool sRGB /* = true */)
	{
		bool bFlipImage = false;

		const std::wstring wpath = ASSETS_TEXTURES_PATH + StringToWString(filePath);
		if (!FileExists(wpath))
		{
			VAST_ASSERTF(0, "Texture file path does not exist.");
			return TextureHandle();
		}

		VAST_PROFILE_TRACE_BEGIN("Load Texture File");
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
		VAST_PROFILE_TRACE_END("Load Texture File");

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

	void GPUResourceManager::UpdateBuffer(BufferHandle h, void* data, const size_t size)
	{
		VAST_PROFILE_TRACE_FUNCTION;
		VAST_ASSERT(h.IsValid());

		gfx::UpdateBuffer(h, data, size);
	}

	void GPUResourceManager::DestroyBuffer(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_BuffersMarkedForDestruction[gfx::GetFrameId()].push_back(h);
	}

	void GPUResourceManager::DestroyTexture(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_TexturesMarkedForDestruction[gfx::GetFrameId()].push_back(h);
	}

	void GPUResourceManager::DestroyPipeline(PipelineHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_PipelinesMarkedForDestruction[gfx::GetFrameId()].push_back(h);
	}

	void GPUResourceManager::ProcessDestructions(uint32 frameId)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		for (auto& h : m_BuffersMarkedForDestruction[frameId])
		{
			VAST_ASSERT(h.IsValid());
			gfx::DestroyBuffer(h);
			m_BufferHandles.FreeHandle(h);
		}
		m_BuffersMarkedForDestruction[frameId].clear();

		for (auto& h : m_TexturesMarkedForDestruction[frameId])
		{
			VAST_ASSERT(h.IsValid());
			gfx::DestroyTexture(h);
			m_TextureHandles.FreeHandle(h);
		}
		m_TexturesMarkedForDestruction[frameId].clear();

		for (auto& h : m_PipelinesMarkedForDestruction[frameId])
		{
			VAST_ASSERT(h.IsValid());
			gfx::DestroyPipeline(h);
			m_PipelineHandles.FreeHandle(h);
		}
		m_PipelinesMarkedForDestruction[frameId].clear();
	}

	ShaderResourceProxy GPUResourceManager::LookupShaderResource(PipelineHandle h, const std::string& shaderResourceName)
	{
		return gfx::LookupShaderResource(h, shaderResourceName);
	}

	void GPUResourceManager::ReloadShaders(PipelineHandle h)
	{
		VAST_ASSERT(h.IsValid());
		m_PipelinesMarkedForShaderReload.push_back(h);
	}

	void GPUResourceManager::ProcessShaderReloads()
	{
		VAST_PROFILE_TRACE_FUNCTION;

		for (auto& h : m_PipelinesMarkedForShaderReload)
		{
			VAST_ASSERT(h.IsValid());
			gfx::ReloadShaders(h);
		}
		m_PipelinesMarkedForShaderReload.clear();
	}

	bool GPUResourceManager::GetIsReady(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetIsReady(h);
	}

	bool GPUResourceManager::GetIsReady(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetIsReady(h);
	}

	const uint8* GPUResourceManager::GetBufferData(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetBufferData(h);
	}

	void GPUResourceManager::SetDebugName(BufferHandle h, const std::string& name)
	{
		VAST_ASSERT(h.IsValid());
		gfx::SetDebugName(h, name);
	}

	void GPUResourceManager::SetDebugName(TextureHandle h, const std::string& name)
	{
		VAST_ASSERT(h.IsValid());
		gfx::SetDebugName(h, name);
	}

	TexFormat GPUResourceManager::GetTextureFormat(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetTextureFormat(h);
	}

	uint32 GPUResourceManager::GetBindlessSRV(BufferHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetBindlessSRV(h);
	}

	uint32 GPUResourceManager::GetBindlessSRV(TextureHandle h)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetBindlessSRV(h);
	}

	uint32 GPUResourceManager::GetBindlessUAV(TextureHandle h, uint32 mipLevel /* = 0 */)
	{
		VAST_ASSERT(h.IsValid());
		return gfx::GetBindlessUAV(h, mipLevel);
	}

	//

	BufferView GPUResourceManager::AllocTempBufferView(uint32 size, uint32 alignment /* = 0 */)
	{
		auto& frameAllocator = m_TempFrameAllocators[gfx::GetFrameId()];
		VAST_ASSERT(frameAllocator.buffer.IsValid() && frameAllocator.bufferSize);

		uint32 allocStart = frameAllocator.Alloc(size, alignment);

		// TODO: We need to attach descriptors to the BufferViews if we want to use them as CBVs, SRVs...
		return BufferView
		{
			// TODO: If we separate handle from data in HandlePool, each BufferView could have its own handle, and we could even generalize BufferViews to just Buffer objects.
			.buffer = frameAllocator.buffer,
			.data = (uint8*)(GetBufferData(frameAllocator.buffer) + allocStart),
			.offset = allocStart,
		};
	}

	uint32 TempAllocator::Alloc(uint32 size, uint32 alignment /* = 0 */)
	{
		VAST_ASSERT(size);
		uint32 offset = usedMemory;
		if (alignment > 0)
		{
			offset = AlignU32(offset, alignment);
		}
		VAST_ASSERT(offset + size <= bufferSize);

		usedMemory += size + alignment; // TODO: Is this just being conservative? Should be += offset delta from alignment + size
		return offset;
	}

	void TempAllocator::Reset()
	{
		usedMemory = 0;
	}

}
