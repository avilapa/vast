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

		constexpr const TextureDesc& GetDesc() const { return desc; }
	};

}