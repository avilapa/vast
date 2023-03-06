#pragma once

#include "Graphics/Graphics.h"
#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

#include "dx12/DirectXAgilitySDK/include/d3d12.h"

namespace D3D12MA
{
	class Allocation;
}

struct IDxcBlob;
struct ID3D12ShaderReflection;

namespace vast::gfx
{
	// - Graphics constants ----------------------------------------------------------------------- //

	constexpr uint32 NUM_FRAMES_IN_FLIGHT = 2;
	constexpr uint32 NUM_BACK_BUFFERS = 3;
	constexpr uint32 NUM_RTV_STAGING_DESCRIPTORS = 256;
	constexpr uint32 NUM_DSV_STAGING_DESCRIPTORS = 32;
	constexpr uint32 NUM_SRV_STAGING_DESCRIPTORS = 4096;
	constexpr uint32 NUM_RESERVED_SRV_DESCRIPTORS = 8192;
	constexpr uint32 NUM_SRV_RENDER_PASS_USER_DESCRIPTORS = 65536;
	constexpr uint32 MAX_QUEUED_BARRIERS = 16;
	// TODO: Set sensible defaults
	constexpr uint32 NUM_TEXTURES = 512;
	constexpr uint32 NUM_BUFFERS = 512;
	constexpr uint32 NUM_PIPELINES = 64;

	constexpr bool ENABLE_VSYNC = true;
	constexpr bool ALLOW_TEARING = false;

	// - Resources -------------------------------------------------------------------------------- //

	static const uint32 kInvalidHeapIdx = UINT32_MAX;

	struct DX12Descriptor
	{
		bool IsValid() const { return cpuHandle.ptr != 0; }
		bool IsReferencedByShader() const { return gpuHandle.ptr != 0; }

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { 0 };
		uint32 heapIdx = 0;
	};

	struct DX12Resource
	{
		ID3D12Resource* resource = nullptr;
		D3D12MA::Allocation* allocation = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		uint32 heapIdx = kInvalidHeapIdx;
		bool isReady = false;
	};

	struct DX12Buffer : public Buffer, public DX12Resource
	{
		uint8* data = nullptr;
		uint32 stride = 0;
		DX12Descriptor cbv = {};
		DX12Descriptor srv = {};
		DX12Descriptor uav = {};

		void SetMappedData(void* p, size_t dataSize)
		{
			const auto size = resource->GetDesc().Width;
			VAST_ASSERT(data != nullptr && p != nullptr && dataSize > 0 && dataSize <= size);
			memcpy_s(data, size, p, dataSize);
			isReady = true; // TODO
		}
	};

	struct DX12Texture : public Texture, public DX12Resource
	{
		DX12Descriptor rtv = {};
		DX12Descriptor dsv = {};
		DX12Descriptor srv = {};
		DX12Descriptor uav = {};
	};

	struct DX12Shader
	{
		IDxcBlob* blob = nullptr;
		ID3D12ShaderReflection* reflection = nullptr;
	};

	struct DX12Pipeline : public Pipeline
	{
		Ptr<DX12Shader> vs = nullptr;
		Ptr<DX12Shader> ps = nullptr;
		ID3D12PipelineState* pipelineState = nullptr;
		ID3D12RootSignature* rootSignature = nullptr;
		ShaderResourceProxyTable resourceProxyTable;
	};

	// - Helpers ---------------------------------------------------------------------------------- //

	inline void DX12Check(HRESULT hr)
	{
		VAST_ASSERT(SUCCEEDED(hr));
	}

	template <typename T>
	inline void DX12SafeRelease(T& p)
	{
		if (p)
		{
			p->Release();
			p = nullptr;
		}
	}

	inline constexpr uint32 AlignU32(uint32 valueToAlign, uint32 alignment)
	{
		alignment -= 1;
		return (uint32)((valueToAlign + alignment) & ~alignment);
	}

	inline constexpr uint64 AlignU64(uint64 valueToAlign, uint64 alignment)
	{
		alignment -= 1;
		return (uint64)((valueToAlign + alignment) & ~alignment);
	}

	constexpr DXGI_FORMAT TranslateToDX12(const Format& v)
	{
		switch (v)
		{
		case Format::UNKNOWN:			return DXGI_FORMAT_UNKNOWN;
		case Format::RG32_FLOAT:		return DXGI_FORMAT_R32G32_FLOAT;
		case Format::RGBA8_UNORM:		return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format::RGBA8_UNORM_SRGB:	return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Format::D32_FLOAT:			return DXGI_FORMAT_D32_FLOAT;
		default: VAST_ASSERTF(0, "Format not supported on this platform."); return DXGI_FORMAT_UNKNOWN;
		}
	}

	constexpr Format TranslateFromDX12(const DXGI_FORMAT& v)
	{
		switch (v)
		{
		case DXGI_FORMAT_UNKNOWN:				return Format::UNKNOWN;
		case DXGI_FORMAT_R32G32_FLOAT:			return Format::RG32_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return Format::RGBA8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:	return Format::RGBA8_UNORM_SRGB;
		case DXGI_FORMAT_D32_FLOAT:				return Format::D32_FLOAT;
		default: VAST_ASSERTF(0, "Unknown Format."); return Format::UNKNOWN;
		}
	}

	constexpr D3D12_RESOURCE_DIMENSION TranslateToDX12(const TextureType& v)
	{
		switch (v)
		{
		case TextureType::UNKNOWN:		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		case TextureType::TEXTURE_1D:	return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TextureType::TEXTURE_2D:	return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TextureType::TEXTURE_3D:	return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default: VAST_ASSERTF(0, "TextureType not supported on this platform."); return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		}
	}

	constexpr TextureType TranslateFromDX12(const D3D12_RESOURCE_DIMENSION& v)
	{
		switch (v)
		{
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:		return TextureType::UNKNOWN;
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:	return TextureType::TEXTURE_1D;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:	return TextureType::TEXTURE_2D;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:	return TextureType::TEXTURE_3D;
		default: VAST_ASSERTF(0, "Unknown TextureType."); return TextureType::UNKNOWN;
		}
	}

	constexpr D3D12_RESOURCE_DESC TranslateToDX12(const TextureDesc& v)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = TranslateToDX12(v.type);
		desc.Alignment = 0;
		desc.Width = static_cast<UINT64>(v.width);
		desc.Height = static_cast<UINT>(v.height);
		desc.DepthOrArraySize = static_cast<UINT16>(v.depthOrArraySize);
		desc.MipLevels = static_cast<UINT16>(v.mipCount);
		desc.Format = TranslateToDX12(v.format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		return desc;
	}

	constexpr TextureDesc TranslateFromDX12_Tex(const D3D12_RESOURCE_DESC& v)
	{
		TextureDesc desc;
		desc.type = TranslateFromDX12(v.Dimension);
		desc.format = TranslateFromDX12(v.Format);
		desc.width = static_cast<uint32>(v.Width);
		desc.height = static_cast<uint32>(v.Height);
		desc.depthOrArraySize = static_cast<uint32>(v.DepthOrArraySize);
		desc.mipCount = static_cast<uint32>(v.MipLevels);
		desc.viewFlags = TextureViewFlags::NONE; // TODO : TextureViewFlags
		return desc;
	}

	constexpr D3D12_RESOURCE_DESC TranslateToDX12(const BufferDesc& v)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0; 
		// TODO: Aligned width only needed for CBV?
		desc.Width = static_cast<UINT64>(AlignU32(static_cast<uint32>(v.size), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		return desc;
	}

	constexpr BufferDesc TranslateFromDX12_Buf(const D3D12_RESOURCE_DESC& v)
	{
		BufferDesc desc;
		desc.size = static_cast<uint32>(v.Width);
		desc.stride = 0; // TODO: Stride
		desc.viewFlags = BufferViewFlags::NONE; // TODO: BufferViewFlags
		desc.accessFlags = BufferAccessFlags::HOST_WRITABLE; // TODO: BufferAccessFlags
		return desc;
	}
}