#pragma once

#include "Graphics/ResourceHandles.h"

namespace vast::gfx
{
	// - Graphics Types --------------------------------------------------------------------------- //

	enum class ClearFlags
	{
		NONE = 0,
		CLEAR_COLOR		= BIT(0),
		CLEAR_DEPTH		= BIT(1),
		CLEAR_STENCIL	= BIT(2),
	};
	ENUM_CLASS_ALLOW_FLAGS(ClearFlags);

	struct ClearParams
	{
		ClearFlags flags = ClearFlags::NONE;
		float4 color = float4(1.0f);
		float depth = 1.0f;
	};

	enum class ResourceUsage // TODO: Update Frequency?
	{
		STATIC,
		DYNAMIC,
	};

	enum class BufferViewFlags
	{
		NONE = 0,
		CBV = BIT(0),
		SRV = BIT(1),
		UAV = BIT(2),
	};
	ENUM_CLASS_ALLOW_FLAGS(BufferViewFlags);

	// TODO: Should CPU writable and Dynamic be exclusive? (cpu write is always dynamic, in which case dynamic would only be used for upload buffers)
	enum class BufferCpuAccess
	{
		NONE,
		WRITE,
		// TODO: READBACK
	};

	enum class Format
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
		R16_UINT,
		COUNT,
	};

	enum class TextureType
	{
		UNKNOWN,
		TEXTURE_1D,
		TEXTURE_2D,
		TEXTURE_3D,
	};

	enum class TextureViewFlags
	{
		NONE = 0,
		RTV = BIT(0),
		DSV = BIT(1),
		SRV = BIT(2),
		UAV = BIT(3),

		RTV_SRV = RTV | SRV,
		DSV_SRV = DSV | SRV,
	};
	ENUM_CLASS_ALLOW_FLAGS(TextureViewFlags);

	enum class ShaderType
	{
		UNKNOWN,
		COMPUTE,
		VERTEX,
		PIXEL,
	};

	enum class CompareFunc
	{
		NONE,
		NEVER,
		LESS,
		EQUAL,
		LESS_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_EQUAL,
		ALWAYS
	};

	enum class Blend
	{
		ZERO,
		ONE,
		SRC_COLOR,
		INV_SRC_COLOR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DST_ALPHA,
		INV_DST_ALPHA,
		DST_COLOR,
		INV_DST_COLOR,
		SRC_ALPHA_SAT,
		BLEND_FACTOR,
		INV_BLEND_FACTOR,
		SRC1_COLOR,
		INV_SRC1_COLOR,
		SRC1_ALPHA,
		INV_SRC1_ALPHA,
		BLEND_ALPHA_FACTOR,
		INV_BLEND_ALPHA_FACTOR,
	};

	enum class BlendOp
	{
		ADD,
		SUBTRACT,
		REV_SUBTRACT,
		MIN,
		MAX,
	};

	enum class ColorWrite
	{
		DISABLE = 0,
		RED,
		GREEN,
		BLUE,
		ALPHA,
		ALL,
	};

	// - Resource Descriptors --------------------------------------------------------------------- //

	// TODO: Provide functions to more easily build common configurations of Desc objects.
	struct BufferDesc
	{
		uint32 size = 0;
		uint32 stride = 0;
		BufferViewFlags viewFlags = BufferViewFlags::NONE;
		BufferCpuAccess cpuAccess = BufferCpuAccess::WRITE;
		ResourceUsage usage = ResourceUsage::STATIC;
		bool isRawAccess = false; // TODO: This refers to using ByteAddressBuffer to read the buffer

		struct Builder;
	};

	struct BufferDesc::Builder
	{
		Builder& Size(uint32 size) { desc.size = size; return *this; }
		Builder& Stride(uint32 stride) { desc.stride = stride; return *this; }
		Builder& ViewFlags(BufferViewFlags viewFlags) { desc.viewFlags = viewFlags; return *this; }
		Builder& CpuAccess(BufferCpuAccess cpuAccess) { desc.cpuAccess = cpuAccess; return *this; }
		Builder& Usage(ResourceUsage usage) { desc.usage = usage; return *this; }
		Builder& IsRawAccess(bool isRawAccess) { desc.isRawAccess = isRawAccess; return *this; }

		operator BufferDesc() { return desc; }
		BufferDesc desc;
	};

	struct TextureDesc
	{
		TextureType type = TextureType::TEXTURE_2D;
		Format format = Format::RGBA8_UNORM_SRGB;
		uint32 width = 1;
		uint32 height = 1;
		uint32 depthOrArraySize = 1;
		uint32 mipCount = 1;
		TextureViewFlags viewFlags = TextureViewFlags::NONE;

		struct Builder;
	};

	struct TextureDesc::Builder
	{
		Builder& Type(TextureType type) { desc.type = type; return *this; }
		Builder& Format(Format format) { desc.format = format; return *this; }
		Builder& Width(uint32 width) { desc.width = width; return *this; }
		Builder& Height(uint32 height) { desc.height = height; return *this; }
		Builder& DepthOrArraySize(uint32 depthOrArraySize) { desc.depthOrArraySize = depthOrArraySize; return *this; }
		Builder& MipCount(uint32 mipCount) { desc.mipCount = mipCount; return *this; }
		Builder& ViewFlags(TextureViewFlags viewFlags) { desc.viewFlags = viewFlags; return *this; }

		operator TextureDesc() { return desc; }
		TextureDesc desc;
	};

	struct ShaderDesc
	{
		ShaderType type = ShaderType::UNKNOWN;
		std::string shaderName;
		std::string entryPoint;
	};

	struct BlendState
	{
		bool blendEnable = false;
		Blend srcBlend = Blend::SRC_ALPHA;
		Blend dstBlend = Blend::INV_SRC_ALPHA;
		BlendOp blendOp = BlendOp::ADD;
		Blend srcBlendAlpha = Blend::ONE;
		Blend dstBlendAlpha = Blend::ONE;
		BlendOp blendOpAlpha = BlendOp::ADD;
		ColorWrite writeMask = ColorWrite::ALL;

		struct Preset;
	};

	struct DepthStencilState
	{
		bool depthEnable = true;
		bool depthWrite = true;
		CompareFunc depthFunc = CompareFunc::LESS_EQUAL;
		// TODO: Stencil state

		struct Preset;
	};

	struct DepthStencilState::Preset
	{
		static constexpr DepthStencilState kDisabled{ false, false,	CompareFunc::LESS_EQUAL };
		static constexpr DepthStencilState kEnabled{ true,  false,	CompareFunc::LESS_EQUAL };
		static constexpr DepthStencilState kEnabledWrite{ true,  true,	CompareFunc::LESS_EQUAL };
	};

	struct BlendState::Preset
	{
		static constexpr BlendState kDisabled{ false };
		static constexpr BlendState kAdditive{ true };
	};

	struct PipelineDesc // TODO: Decide on a name for our pipeline/render pass
	{
		ShaderDesc vs;
		ShaderDesc ps;
		DepthStencilState depthStencilState;
		Array<BlendState, 8> rtBlendStates; // TODO: Define 8 (D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
		Array<Format, 8> rtFormats = { Format::UNKNOWN }; // TODO: Define 8 (D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
		uint8 rtCount = 0;

		struct Builder;
	};

	struct PipelineDesc::Builder
	{
		Builder& VS(const std::string& fileName, const std::string& entryPoint) { desc.vs = ShaderDesc{ ShaderType::VERTEX, fileName, entryPoint }; return *this; }
 		Builder& PS(const std::string& fileName, const std::string& entryPoint) { desc.ps = ShaderDesc{ ShaderType::PIXEL,  fileName, entryPoint }; return *this; }
		Builder& DepthStencil(DepthStencilState ds) { desc.depthStencilState = ds; return *this; }
		Builder& SetRenderTarget(Format format, BlendState bs = BlendState::Preset::kDisabled) { desc.rtFormats[desc.rtCount] = format; desc.rtBlendStates[desc.rtCount] = bs; ++desc.rtCount; return *this; }

		operator PipelineDesc() { return desc; }
		PipelineDesc desc;
	};
}