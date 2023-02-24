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


	enum class ResourceType
	{
		UNKNOWN,
		BUFFER,
		TEXTURE,
		SHADER,
		COUNT,
	};
	constexpr char* g_ResourceTypeNames[]
	{
		"Unknown",
		"Buffer",
		"Texture",
		"Shader",
	};
	static_assert(NELEM(g_ResourceTypeNames) == IDX(ResourceType::COUNT));

	enum class ResourceState
	{
		RENDER_TARGET,
		PRESENT,
	};

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
	};

	struct Texture
	{
		static constexpr char* GetResourceTypeName() { return g_ResourceTypeNames[IDX(ResourceType::TEXTURE)]; }
		// TODO: dummy for now
	};

	struct BufferDesc
	{
		uint32 size = 0;
		uint32 stride = 0;
		BufferViewFlags viewFlags = BufferViewFlags::NONE;
		BufferAccessFlags accessFlags = BufferAccessFlags::HOST_WRITABLE;
		bool isRawAccess = false; // TODO: This refers to using ByteAddressBuffer to read the buffer
	};

	struct Buffer
	{
		static constexpr char* GetResourceTypeName() { return g_ResourceTypeNames[IDX(ResourceType::BUFFER)]; }
		// TODO: dummy for now
	};

	struct ShaderDesc
	{
		ShaderType type = ShaderType::COMPUTE;
		std::wstring shaderName;
		std::wstring entryPoint;
	};

	struct Shader
	{
		static constexpr char* GetResourceTypeName() { return g_ResourceTypeNames[IDX(ResourceType::SHADER)]; }
		// TODO: dummy for now
	};
}