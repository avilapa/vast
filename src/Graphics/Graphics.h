#pragma once

namespace vast::gfx
{

	enum class Format
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
		COUNT,
	};

	enum class ResourceState
	{
		RENDER_TARGET,
		PRESENT,
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

	enum class ResourceType
	{
		UNKNOWN,
		BUFFER,
		TEXTURE,
	};

	struct GPUResource
	{
		ResourceType type = ResourceType::UNKNOWN;
		Ref<void> internalState;

		inline bool IsValid() const { return internalState.get() != nullptr; }
		constexpr bool IsTexture() const { return type == ResourceType::TEXTURE; }
		constexpr bool IsBuffer() const { return type == ResourceType::BUFFER; }
		virtual ~GPUResource() = default;
	};

	struct BufferDesc
	{
		uint32 size = 0;
		uint32 stride = 0;
		BufferViewFlags viewFlags = BufferViewFlags::NONE;
		BufferAccessFlags accessFlags = BufferAccessFlags::HOST_WRITABLE;
		bool isRawAccess = false; // TODO: This refers to using ByteAddressBuffer to read the buffer
	};

	struct Buffer : public GPUResource
	{
		Buffer() : GPUResource() { type = ResourceType::BUFFER; }
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
	};

	struct Texture : public GPUResource
	{
		Texture() : GPUResource() { type = ResourceType::TEXTURE; }
		TextureDesc desc;
	};

}