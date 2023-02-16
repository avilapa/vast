#pragma once

#include "Graphics/graphics_context.h"
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

	// TODO: Move this to descriptors
	struct DX12DescriptorHandle
	{
		bool IsValid() const { return m_CPUHandle.ptr != 0; }
		bool IsReferencedByShader() const { return m_GPUHandle.ptr != 0; }

		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle = { 0 };
		uint32 m_HeapIndex = 0;
	};

	// TODO: Move resources
	struct Resource
	{
		enum class Type : bool
		{
			BUFFER = 0,
			TEXTURE = 1
		};

		Type m_Type = Type::BUFFER;

		ID3D12Resource* m_Resource = nullptr;
		// TODO: D3D12MA::Allocation* m_Allocation = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress = 0;
		D3D12_RESOURCE_DESC m_Desc = {};
		D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
		uint32 m_HeapIndex = UINT32_MAX; // Invalid value
		bool m_IsReady = false;
	};

	struct Texture : public Resource
	{
		Texture() : Resource() { m_Type = Type::TEXTURE; }

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

	static DXGI_FORMAT TranslateToDX12(Format e)
	{
		switch (e)
		{
		case Format::UNKNOWN:			return DXGI_FORMAT_UNKNOWN;
		case Format::RG32_FLOAT:		return DXGI_FORMAT_R32G32_FLOAT;
		case Format::RGBA8_UNORM:		return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format::RGBA8_UNORM_SRGB:	return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Format::D32_FLOAT:			return DXGI_FORMAT_D32_FLOAT;
		default: VAST_ASSERTF(0, "Format type not supported."); return DXGI_FORMAT_UNKNOWN;
		}
	}

	static D3D12_RESOURCE_STATES TranslateToDX12(ResourceState e)
	{
		switch (e)
		{
		case ResourceState::RENDER_TARGET:	return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::PRESENT:		return D3D12_RESOURCE_STATE_PRESENT;
		default: VAST_ASSERTF(0, "ResourceState type not supported."); return D3D12_RESOURCE_STATE_COMMON;
		}
	}
}