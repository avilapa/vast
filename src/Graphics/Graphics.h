#pragma once

#include "Graphics/ResourceHandles.h"

namespace vast::gfx
{
	// - Graphics Enums --------------------------------------------------------------------------- //

	enum class Format
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
		COUNT,
	};

	enum class BufferViewFlags
	{
		NONE = 0,
		CBV = BIT(0),
		SRV = BIT(1),
		UAV = BIT(2),
	};
	ENUM_CLASS_ALLOW_FLAGS(BufferViewFlags);

	enum class BufferAccessFlags
	{
		HOST_WRITABLE	= BIT(0),
		GPU_ONLY		= BIT(1),
	};
	ENUM_CLASS_ALLOW_FLAGS(BufferAccessFlags);

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
		COMPUTE,
		VERTEX,
		PIXEL,
	};

	// - Resource Descriptors --------------------------------------------------------------------- //

	// TODO: Provide functions to more easily build common configurations of Desc objects.
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

	struct BufferDesc
	{
		uint32 size = 0;
		uint32 stride = 0;
		BufferViewFlags viewFlags = BufferViewFlags::NONE;
		BufferAccessFlags accessFlags = BufferAccessFlags::HOST_WRITABLE;
		bool isRawAccess = false; // TODO: This refers to using ByteAddressBuffer to read the buffer

		struct Builder;
	};

	struct ShaderDesc
	{
		ShaderType type = ShaderType::COMPUTE;
		std::wstring shaderName;
		std::wstring entryPoint;

		struct Builder;
	};

	struct PipelineDesc // TODO: Decide on a name for our pipeline/render pass
	{
		ResourceHandle<class Shader> vs;
		ResourceHandle<class Shader> ps;
		uint8 rtCount = 0;
		Array<Format, 8> rtFormats; // TODO: Define 8 (D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
		// TODO: Depth Stencil format
		// TODO: Pipeline state stuff

		struct Builder;
	};

	struct TextureDesc::Builder
	{
		Builder& TextureType(const TextureType& type) { desc.type = type; return *this; }
		Builder& Format(const Format& format) { desc.format = format; return *this; }
		Builder& Width(const uint32& width) { desc.width = width; return *this; }
		Builder& Height(const uint32& height) { desc.height = height; return *this; }
		Builder& DepthOrArraySize(const uint32& depthOrArraySize) { desc.depthOrArraySize = depthOrArraySize; return *this; }
		Builder& MipCount(const uint32& mipCount) { desc.mipCount = mipCount; return *this; }
		Builder& ViewFlags(const TextureViewFlags& viewFlags) { desc.viewFlags = viewFlags; return *this; }

		operator TextureDesc() { return desc; }
		TextureDesc desc;
	};

	struct BufferDesc::Builder
	{
		Builder& Size(const uint32& size) { desc.size = size; return *this; }
		Builder& Stride(const uint32& stride) { desc.stride = stride; return *this; }
		Builder& ViewFlags(const BufferViewFlags& viewFlags) { desc.viewFlags = viewFlags; return *this; }
		Builder& AccessFlags(const BufferAccessFlags& accessFlags) { desc.accessFlags = accessFlags; return *this; }
		Builder& IsRawAccess(const bool& isRawAccess) { desc.isRawAccess = isRawAccess; return *this; }

		operator BufferDesc() { return desc; }
		BufferDesc desc;
	};

	struct ShaderDesc::Builder
	{
		Builder& Type(const ShaderType& type) { desc.type = type; return *this; }
		Builder& ShaderName(const std::wstring& shaderName) { desc.shaderName = shaderName; return *this; }
		Builder& EntryPoint(const std::wstring& entryPoint) { desc.entryPoint = entryPoint; return *this; }

		operator ShaderDesc() { return desc; }
		ShaderDesc desc;
	};

	struct PipelineDesc::Builder
	{
		Builder& VS(const ResourceHandle<class Shader>& vs) { desc.vs = vs; return *this; }
		Builder& PS(const ResourceHandle<class Shader>& ps) { desc.vs = ps; return *this; }
		Builder& SetRenderTarget(const Format& format) { desc.rtFormats[desc.rtCount++] = format; return *this; }

		operator PipelineDesc() { return desc; }
		PipelineDesc desc;
	};
}