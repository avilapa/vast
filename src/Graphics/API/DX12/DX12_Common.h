#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

#include "dx12/DirectXAgilitySDK/include/d3d12.h"
#include <dxgi1_6.h>

namespace D3D12MA
{
	class Allocation;
}

struct IDxcBlob;
struct ID3D12ShaderReflection;

namespace vast::gfx
{
	static_assert(CONSTANT_BUFFER_ALIGNMENT == D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	static_assert(MAX_RENDERTARGETS == D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	// - Resources -------------------------------------------------------------------------------- //

	static const uint32 kInvalidHeapIdx = UINT32_MAX;

	struct DX12Descriptor
	{
		bool IsValid() const { return cpuHandle.ptr != 0; }
		bool IsReferencedByShader() const { return gpuHandle.ptr != 0; }

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { 0 };
		uint32 heapIdx = 0;
		uint32 bindlessIdx = kInvalidHeapIdx;
	};

	struct DX12Resource
	{
		ID3D12Resource* resource = nullptr;
		D3D12MA::Allocation* allocation = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		bool isReady = false;

		virtual void Reset()
		{
			resource = nullptr;
			allocation = nullptr;
			gpuAddress = 0;
			state = D3D12_RESOURCE_STATE_COMMON;
			isReady = false;
		}

		void SetName(const std::string& name)
		{
#ifdef VAST_DEBUG
			resource->SetName(std::wstring(name.begin(), name.end()).c_str());
#else
			(void)name;
#endif
		}

		std::string GetName()
		{
#ifdef VAST_DEBUG
			wchar_t wname[128] = {};
			UINT size = sizeof(wname);
			resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, wname);
			char name[128] = {};
			char DefChar = 'X';
			WideCharToMultiByte(CP_ACP, 0, wname, -1, name, 128, &DefChar, NULL);
			return std::string(name);
#else
			return std::string("Unnamed");
#endif
		}
	};

	struct DX12Buffer : public Buffer, public DX12Resource
	{
		uint8* data = nullptr;
		uint32 stride = 0;
		ResourceUsage usage = ResourceUsage::DEFAULT;
		DX12Descriptor cbv = {};
		DX12Descriptor srv = {};
		DX12Descriptor uav = {};

		void Reset() override
		{
			data = nullptr;
			stride = 0;
			usage = ResourceUsage::DEFAULT;
			cbv = {};
			srv = {};
			uav = {};
			DX12Resource::Reset();
		}
	};

	struct DX12Texture : public Texture, public DX12Resource
	{
		DX12Descriptor rtv = {};
		DX12Descriptor dsv = {};
		DX12Descriptor srv = {};
		Vector<DX12Descriptor> uav = {};
		D3D12_CLEAR_VALUE clearValue = {};

		void Reset() override
		{
			rtv = {};
			dsv = {};
			srv = {};
			uav = {};
			clearValue = {};
			DX12Resource::Reset();
		}
	};

	struct DX12Shader
	{
		std::string key; // TODO: This is redundant, and could just store a key instead of a full ref on the Pipeline.
		IDxcBlob* blob = nullptr;
		ID3D12ShaderReflection* reflection = nullptr;
	};

	struct DX12Pipeline : public Pipeline
	{
		Ref<DX12Shader> vs = nullptr;
		Ref<DX12Shader> ps = nullptr;
		Ref<DX12Shader> cs = nullptr;
		ID3D12PipelineState* pipelineState = nullptr;
		ID3D12RootSignature* rootSignature = nullptr;
		// Note: We store the desc to be able to reload the pipeline without having to fully 
		// reconstruct it from user land.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

		ShaderResourceProxyTable resourceProxyTable;
		uint8 pushConstantIndex = UINT8_MAX;
		uint8 descriptorTableIndex = UINT8_MAX;

		bool IsCompute() const { return cs != nullptr; }

		void Reset()
		{
			desc = {};
			resourceProxyTable.Reset();
			pushConstantIndex = UINT8_MAX;
			descriptorTableIndex = UINT8_MAX;
		}
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

	constexpr DXGI_GPU_PREFERENCE TranslateToDX12(const GPUAdapterPreferenceCriteria& v)
	{
		switch (v)
		{
		case GPUAdapterPreferenceCriteria::MAX_PERF:	return DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
		case GPUAdapterPreferenceCriteria::MAX_VRAM:	return DXGI_GPU_PREFERENCE_UNSPECIFIED;
		case GPUAdapterPreferenceCriteria::MIN_POWER:	return DXGI_GPU_PREFERENCE_MINIMUM_POWER;
		default: VAST_ASSERTF(0, "Invalid GPUAdapterPreferenceCriteria."); return DXGI_GPU_PREFERENCE_UNSPECIFIED;
		}
	}

	constexpr DXGI_FORMAT TranslateToDX12(const IndexBufFormat& v)
	{
		switch (v)
		{
		case IndexBufFormat::UNKNOWN:	return DXGI_FORMAT_UNKNOWN;
		case IndexBufFormat::R16_UINT:	return DXGI_FORMAT_R16_UINT;
		default: VAST_ASSERTF(0, "IndexBufFormat not supported on this platform."); return DXGI_FORMAT_UNKNOWN;
		}
	}

	constexpr DXGI_FORMAT TranslateToDX12(const TexFormat& v)
	{
		switch (v)
		{
		case TexFormat::UNKNOWN:				return DXGI_FORMAT_UNKNOWN;
		case TexFormat::RGBA32_FLOAT:			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case TexFormat::RGBA16_FLOAT:			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case TexFormat::RG32_FLOAT:				return DXGI_FORMAT_R32G32_FLOAT;
		case TexFormat::RG16_FLOAT:				return DXGI_FORMAT_R16G16_FLOAT;

		case TexFormat::RGBA8_UNORM:			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TexFormat::RGBA8_UNORM_SRGB:		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		case TexFormat::D16_UNORM:				return DXGI_FORMAT_D16_UNORM;
		case TexFormat::D32_FLOAT:				return DXGI_FORMAT_D32_FLOAT;
		case TexFormat::D32_FLOAT_S8X24_UINT:	return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case TexFormat::D24_UNORM_S8_UINT:		return DXGI_FORMAT_D24_UNORM_S8_UINT;

		case TexFormat::R16_UINT:				return DXGI_FORMAT_R16_UINT;
		default: VAST_ASSERTF(0, "TexFormat not supported on this platform."); return DXGI_FORMAT_UNKNOWN;
		}
	}

	constexpr TexFormat TranslateFromDX12(const DXGI_FORMAT& v)
	{
		switch (v)
		{
		case DXGI_FORMAT_UNKNOWN:				return TexFormat::UNKNOWN;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return TexFormat::RGBA32_FLOAT;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return TexFormat::RGBA16_FLOAT;
		case DXGI_FORMAT_R32G32_FLOAT:			return TexFormat::RG32_FLOAT;
		case DXGI_FORMAT_R16G16_FLOAT:			return TexFormat::RG16_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return TexFormat::RGBA8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:	return TexFormat::RGBA8_UNORM_SRGB;

		case DXGI_FORMAT_D16_UNORM:				return TexFormat::D16_UNORM;
		case DXGI_FORMAT_D32_FLOAT:				return TexFormat::D32_FLOAT;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:	return TexFormat::D32_FLOAT_S8X24_UINT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:		return TexFormat::D24_UNORM_S8_UINT;

		case DXGI_FORMAT_R16_UINT:				return TexFormat::R16_UINT;
		default: VAST_ASSERTF(0, "Unknown TexFormat."); return TexFormat::UNKNOWN;
		}
	}

	constexpr D3D12_RESOURCE_DIMENSION TranslateToDX12(const TexType& v)
	{
		switch (v)
		{
		case TexType::UNKNOWN:		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		case TexType::TEXTURE_1D:	return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TexType::TEXTURE_2D:	return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TexType::TEXTURE_3D:	return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default: VAST_ASSERTF(0, "TexType not supported on this platform."); return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		}
	}

	constexpr TexType TranslateFromDX12(const D3D12_RESOURCE_DIMENSION& v)
	{
		switch (v)
		{
		case D3D12_RESOURCE_DIMENSION_UNKNOWN:		return TexType::UNKNOWN;
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:	return TexType::TEXTURE_1D;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:	return TexType::TEXTURE_2D;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:	return TexType::TEXTURE_3D;
		default: VAST_ASSERTF(0, "Unknown TexType."); return TexType::UNKNOWN;
		}
	}

	constexpr D3D12_COMPARISON_FUNC TranslateToDX12(const CompareFunc& v)
	{
		switch (v)
		{
		case CompareFunc::NONE:				return D3D12_COMPARISON_FUNC_NONE;
		case CompareFunc::NEVER:			return D3D12_COMPARISON_FUNC_NEVER;
		case CompareFunc::LESS:				return D3D12_COMPARISON_FUNC_LESS;
		case CompareFunc::EQUAL:			return D3D12_COMPARISON_FUNC_EQUAL;
		case CompareFunc::LESS_EQUAL:		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case CompareFunc::GREATER:			return D3D12_COMPARISON_FUNC_GREATER;
		case CompareFunc::NOT_EQUAL:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case CompareFunc::GREATER_EQUAL:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case CompareFunc::ALWAYS:			return D3D12_COMPARISON_FUNC_ALWAYS;
		default: VAST_ASSERTF(0, "CompareFunc not supported on this platform."); return D3D12_COMPARISON_FUNC_NONE;
		}
	}

	constexpr D3D12_FILL_MODE TranslateToDX12(const FillMode& v)
	{
		switch (v)
		{
		case FillMode::SOLID:		return D3D12_FILL_MODE_SOLID;
		case FillMode::WIREFRAME:	return D3D12_FILL_MODE_WIREFRAME;
		default: VAST_ASSERTF(0, "FillMode not supported on this platform."); return D3D12_FILL_MODE_SOLID;
		}
	}

	constexpr D3D12_CULL_MODE TranslateToDX12(const CullMode& v)
	{
		switch (v)
		{
		case CullMode::NONE:	return D3D12_CULL_MODE_NONE;
		case CullMode::FRONT:	return D3D12_CULL_MODE_FRONT;
		case CullMode::BACK:	return D3D12_CULL_MODE_BACK;
		default: VAST_ASSERTF(0, "CullMode not supported on this platform."); return D3D12_CULL_MODE_NONE;
		}
	}

	constexpr D3D12_BLEND TranslateToDX12(const Blend& v)
	{
		switch (v)
		{
		case Blend::ZERO:					return D3D12_BLEND_ZERO;
		case Blend::ONE:					return D3D12_BLEND_ONE;
		case Blend::SRC_COLOR:				return D3D12_BLEND_SRC_COLOR;
		case Blend::INV_SRC_COLOR:			return D3D12_BLEND_INV_SRC_COLOR;
		case Blend::SRC_ALPHA:				return D3D12_BLEND_SRC_ALPHA;
		case Blend::INV_SRC_ALPHA:			return D3D12_BLEND_INV_SRC_ALPHA;
		case Blend::DST_ALPHA:				return D3D12_BLEND_DEST_ALPHA;
		case Blend::INV_DST_ALPHA:			return D3D12_BLEND_INV_DEST_ALPHA;
		case Blend::DST_COLOR:				return D3D12_BLEND_DEST_COLOR;
		case Blend::INV_DST_COLOR:			return D3D12_BLEND_INV_DEST_COLOR;
		case Blend::SRC_ALPHA_SAT:			return D3D12_BLEND_SRC_ALPHA_SAT;
		case Blend::BLEND_FACTOR:			return D3D12_BLEND_BLEND_FACTOR;
		case Blend::INV_BLEND_FACTOR:		return D3D12_BLEND_INV_BLEND_FACTOR;
		case Blend::SRC1_COLOR:				return D3D12_BLEND_SRC1_COLOR;
		case Blend::INV_SRC1_COLOR:			return D3D12_BLEND_INV_SRC1_COLOR;
		case Blend::SRC1_ALPHA:				return D3D12_BLEND_SRC1_ALPHA;
		case Blend::INV_SRC1_ALPHA:			return D3D12_BLEND_INV_SRC1_ALPHA;
		case Blend::BLEND_ALPHA_FACTOR:		return D3D12_BLEND_ALPHA_FACTOR;
		case Blend::INV_BLEND_ALPHA_FACTOR:	return D3D12_BLEND_INV_ALPHA_FACTOR;
		default: VAST_ASSERTF(0, "Blend type not supported on this platform."); return D3D12_BLEND_ZERO;
		}
	}

	constexpr D3D12_BLEND_OP TranslateToDX12(const BlendOp& v)
	{
		switch (v)
		{
		case BlendOp::ADD:			return D3D12_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:		return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::REV_SUBTRACT:	return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOp::MIN:			return D3D12_BLEND_OP_MIN;
		case BlendOp::MAX:			return D3D12_BLEND_OP_MAX;
		default: VAST_ASSERTF(0, "Blend op not supported on this platform."); return D3D12_BLEND_OP_ADD;
		}
	}
	
	constexpr UINT8 TranslateToDX12(const ColorWrite& v)
	{
		switch (v)
		{
		case ColorWrite::DISABLE:	return 0;
		case ColorWrite::RED:		return D3D12_COLOR_WRITE_ENABLE_RED;
		case ColorWrite::GREEN:		return D3D12_COLOR_WRITE_ENABLE_GREEN;
		case ColorWrite::BLUE:		return D3D12_COLOR_WRITE_ENABLE_BLUE;
		case ColorWrite::ALPHA:		return D3D12_COLOR_WRITE_ENABLE_ALPHA;
		case ColorWrite::ALL:		return D3D12_COLOR_WRITE_ENABLE_ALL;
		default: VAST_ASSERTF(0, "Color write mask not supported on this platform."); return 0;
		}
	}

	constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE TranslateToDX12(const LoadOp& v)
	{
		switch (v)
		{
		case LoadOp::LOAD:		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
		case LoadOp::CLEAR:		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		case LoadOp::DISCARD:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
		default: VAST_ASSERTF(0, "Load Operation not supported on this platform."); return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
		}
	}
	
	constexpr D3D12_RENDER_PASS_ENDING_ACCESS_TYPE TranslateToDX12(const StoreOp& v)
	{
		switch (v)
		{
		case StoreOp::STORE:	return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		case StoreOp::DISCARD:	return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		default: VAST_ASSERTF(0, "Store Operation not supported on this platform."); return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		}
	}

	constexpr D3D12_RESOURCE_STATES TranslateToDX12(const ResourceState& v)
	{
		switch (v)
		{
		case ResourceState::NONE:						return D3D12_RESOURCE_STATE_COMMON;
		case ResourceState::PIXEL_SHADER_RESOURCE:		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case ResourceState::NON_PIXEL_SHADER_RESOURCE:	return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case ResourceState::UNORDERED_ACCESS:			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

		case ResourceState::RENDER_TARGET:				return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::DEPTH_WRITE:				return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case ResourceState::DEPTH_READ:					return D3D12_RESOURCE_STATE_DEPTH_READ;

		case ResourceState::VERTEX_BUFFER:				return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case ResourceState::INDEX_BUFFER:				return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case ResourceState::CONSTANT_BUFFER:			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		default:
		{
			D3D12_RESOURCE_STATES ret = {};
			if ((v & ResourceState::PIXEL_SHADER_RESOURCE) == ResourceState::PIXEL_SHADER_RESOURCE)
				ret |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			if ((v & ResourceState::NON_PIXEL_SHADER_RESOURCE) == ResourceState::NON_PIXEL_SHADER_RESOURCE)
				ret |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			if ((v & ResourceState::UNORDERED_ACCESS) == ResourceState::UNORDERED_ACCESS)
				ret |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

			if ((v & ResourceState::RENDER_TARGET) == ResourceState::RENDER_TARGET)
				ret |= D3D12_RESOURCE_STATE_RENDER_TARGET;
			if ((v & ResourceState::DEPTH_WRITE) == ResourceState::DEPTH_WRITE)
				ret |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
			if ((v & ResourceState::DEPTH_READ) == ResourceState::DEPTH_READ)
				ret |= D3D12_RESOURCE_STATE_DEPTH_READ;

			if ((v & ResourceState::VERTEX_BUFFER) == ResourceState::VERTEX_BUFFER)
				ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			if ((v & ResourceState::INDEX_BUFFER) == ResourceState::INDEX_BUFFER)
				ret |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
			if ((v & ResourceState::CONSTANT_BUFFER) == ResourceState::CONSTANT_BUFFER)
				ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			return ret;
		}
		}
	}

	constexpr ResourceState TranslateFromDX12(const D3D12_RESOURCE_STATES& v)
	{
		switch (v)
		{
		case D3D12_RESOURCE_STATE_COMMON:						return ResourceState::NONE;
		case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:		return ResourceState::PIXEL_SHADER_RESOURCE;
		case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:	return ResourceState::NON_PIXEL_SHADER_RESOURCE;
		case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:				return ResourceState::UNORDERED_ACCESS;
		case D3D12_RESOURCE_STATE_RENDER_TARGET:				return ResourceState::RENDER_TARGET;
		case D3D12_RESOURCE_STATE_DEPTH_WRITE:					return ResourceState::DEPTH_WRITE;
		case D3D12_RESOURCE_STATE_DEPTH_READ:					return ResourceState::DEPTH_READ;
		case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:	return ResourceState::VERTEX_BUFFER | ResourceState::CONSTANT_BUFFER;
		case D3D12_RESOURCE_STATE_INDEX_BUFFER:					return ResourceState::INDEX_BUFFER;
		default:
		{
			ResourceState ret = {};
			if ((v & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				ret = ret | ResourceState::PIXEL_SHADER_RESOURCE;
			if ((v & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
				ret = ret | ResourceState::NON_PIXEL_SHADER_RESOURCE;
			if ((v & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				ret = ret | ResourceState::UNORDERED_ACCESS;

			if ((v & D3D12_RESOURCE_STATE_RENDER_TARGET) == D3D12_RESOURCE_STATE_RENDER_TARGET)
				ret = ret | ResourceState::RENDER_TARGET;
			if ((v & D3D12_RESOURCE_STATE_DEPTH_WRITE) == D3D12_RESOURCE_STATE_DEPTH_WRITE)
				ret = ret | ResourceState::DEPTH_WRITE;
			if ((v & D3D12_RESOURCE_STATE_DEPTH_READ) == D3D12_RESOURCE_STATE_DEPTH_READ)
				ret = ret | ResourceState::DEPTH_READ;

			if ((v & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
				ret = ret | ResourceState::VERTEX_BUFFER;
			if ((v & D3D12_RESOURCE_STATE_INDEX_BUFFER) == D3D12_RESOURCE_STATE_INDEX_BUFFER)
				ret = ret | ResourceState::INDEX_BUFFER;
			if ((v & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
				ret = ret | ResourceState::CONSTANT_BUFFER;
			return ret;
		}
		}
	}

	constexpr D3D12_RESOURCE_DESC TranslateToDX12(const BufferDesc& v)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0; 
		desc.Width = static_cast<UINT64>(v.size);
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

}