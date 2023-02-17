#pragma once

#include "Graphics/GraphicsContext.h"

#include "dx12/DirectXAgilitySDK/include/d3d12.h"

namespace vast::gfx
{
	// Graphics constants
	constexpr uint32 NUM_FRAMES_IN_FLIGHT = 2;
	constexpr uint32 NUM_BACK_BUFFERS = 3;
	constexpr uint32 NUM_RTV_STAGING_DESCRIPTORS = 256;
	constexpr uint32 NUM_SRV_STAGING_DESCRIPTORS = 4096;
	constexpr uint32 NUM_RESERVED_SRV_DESCRIPTORS = 8192;
	constexpr uint32 NUM_SRV_RENDER_PASS_USER_DESCRIPTORS = 65536;
	constexpr uint32 MAX_QUEUED_BARRIERS = 16;

	constexpr bool ENABLE_VSYNC = true;
	constexpr bool ALLOW_TEARING = false;

	// TODO: Move this to descriptors
	struct DX12DescriptorHandle
	{
		bool IsValid() const { return m_CPUHandle.ptr != 0; }
		bool IsReferencedByShader() const { return m_GPUHandle.ptr != 0; }

		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle = { 0 };
		uint32 m_HeapIndex = 0;
	};

	struct DX12Resource
	{
		ResourceType m_Type = ResourceType::BUFFER;

		ID3D12Resource* m_Resource = nullptr;
		// TODO: D3D12MA::Allocation* m_Allocation = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress = 0;
		D3D12_RESOURCE_DESC m_Desc = {}; // TODO: Do we need m_Desc? We can get it from m_Resource->GetDesc() after all.
		D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
		uint32 m_HeapIndex = UINT32_MAX; // Invalid value
		bool m_IsReady = false;
	};

	struct DX12Texture : public DX12Resource
	{
		DX12Texture() : DX12Resource() { m_Type = ResourceType::TEXTURE; }

		DX12DescriptorHandle m_RTVDescriptor = {};
		DX12DescriptorHandle m_DSVDescriptor = {};
		DX12DescriptorHandle m_SRVDescriptor = {};
		DX12DescriptorHandle m_UAVDescriptor = {};
	};

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

	constexpr DXGI_FORMAT TranslateToDX12(Format v)
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

	constexpr Format TranslateFromDX12(DXGI_FORMAT v)
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

	constexpr D3D12_RESOURCE_STATES TranslateToDX12(ResourceState v)
	{
		switch (v)
		{
		case ResourceState::RENDER_TARGET:	return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::PRESENT:		return D3D12_RESOURCE_STATE_PRESENT;
		default: VAST_ASSERTF(0, "ResourceState not supported on this platform."); return D3D12_RESOURCE_STATE_COMMON;
		}
	}

	constexpr D3D12_RESOURCE_DIMENSION TranslateToDX12(TextureType v)
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

	constexpr TextureType TranslateFromDX12(D3D12_RESOURCE_DIMENSION v)
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

	constexpr D3D12_RESOURCE_DESC  TranslateToDX12(TextureDesc v)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = TranslateToDX12(v.type);
		desc.Format = TranslateToDX12(v.format);
		desc.Width = v.width;
		desc.Height = v.height;
		desc.DepthOrArraySize = v.depthOrArraySize;
		desc.MipLevels = v.mipCount;
		// TODO: TextureViewFlags
		return desc;
	}

	constexpr TextureDesc TranslateFromDX12(D3D12_RESOURCE_DESC v)
	{
		TextureDesc t;
		t.type = TranslateFromDX12(v.Dimension);
		t.format = TranslateFromDX12(v.Format);
		t.width = v.Width;
		t.height = v.Height;
		t.depthOrArraySize = v.DepthOrArraySize;
		t.mipCount = v.MipLevels;
		t.viewFlags = TextureViewFlags::NONE; // TODO : TextureViewFlags
		return t;
	}
}