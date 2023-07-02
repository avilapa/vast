#pragma once

#include "Core/Handles.h"
#include "Graphics/GraphicsTypes.h"

namespace vast::gfx
{
	// - Resource Handles ------------------------------------------------------------------------- //

	using BufferHandle = Handle<class Buffer>;
	using TextureHandle = Handle<class Texture>;
	using PipelineHandle = Handle<class Pipeline>;

	// - Resource Descriptors --------------------------------------------------------------------- //

	struct BufferDesc
	{
		uint32 size = 0;
		uint32 stride = 0;
		BufViewFlags viewFlags = BufViewFlags::NONE;
		BufCpuAccess cpuAccess = BufCpuAccess::WRITE;
		ResourceUsage usage = ResourceUsage::STATIC;
		bool isRawAccess = false; // TODO: This refers to using ByteAddressBuffer to read the buffer
	};

	// TODO: Currently usage is tied to CPU access, need to better figure out a general approach to
	// Dynamic buffers
	BufferDesc AllocVertexBufferDesc(uint32 size, uint32 stride, bool bCpuAccess = false, bool bBindless = true);
	BufferDesc AllocIndexBufferDesc(uint32 numIndices, IndexBufFormat format = IndexBufFormat::R16_UINT);
	BufferDesc AllocCbvBufferDesc(uint32 size, bool bCpuAccess = true);

	struct BufferView
	{
		BufferHandle buffer;
		uint8* data;
		uint32 offset;
	};

	struct TextureDesc
	{
		TexType type = TexType::TEXTURE_2D;
		TexFormat format = TexFormat::RGBA8_UNORM_SRGB;
		uint32 width = 1;
		uint32 height = 1;
		uint32 depthOrArraySize = 1;
		uint32 mipCount = 1;
		TexViewFlags viewFlags = TexViewFlags::NONE;
		ClearValue clear = ClearValue(float4(1.0f, 1.0f, 1.0f, 1.0f));
	};

	TextureDesc AllocRenderTargetDesc(TexFormat format, uint2 dimensions, float4 clear = float4(0, 0, 0, 1));
	TextureDesc AllocDepthStencilTargetDesc(TexFormat format, uint2 dimensions, ClearDepthStencil clear = { 1.0f, 0 });

	struct ShaderDesc
	{
		ShaderType type = ShaderType::UNKNOWN;
		std::string shaderName = "";
		std::string entryPoint = "";
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
		static constexpr DepthStencilState kDisabled{ false, false, CompareFunc::LESS_EQUAL };
		static constexpr DepthStencilState kEnabled{ true, false, CompareFunc::LESS_EQUAL };
		static constexpr DepthStencilState kEnabledWrite{ true, true, CompareFunc::LESS_EQUAL };
	};

	struct RasterizerState
	{
		FillMode fillMode = FillMode::SOLID;
		CullMode cullMode = CullMode::NONE;
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

	struct BlendState::Preset
	{
		static constexpr BlendState kDisabled{ false };
		static constexpr BlendState kAdditive{ true };
	};

	static constexpr uint32 MAX_RENDERTARGETS = 8;

	struct RenderPassLayout
	{
		Array<TexFormat, MAX_RENDERTARGETS> rtFormats;
		TexFormat dsFormat;
	};

	struct PipelineDesc
	{
		ShaderDesc vs = {};
		ShaderDesc ps = {};
		Array<BlendState, MAX_RENDERTARGETS> blendStates = {};
		DepthStencilState depthStencilState = {};
		RasterizerState rasterizerState = {};
		RenderPassLayout renderPassLayout = {};
	};

	struct RenderTargetDesc
	{
		TextureHandle h;
		LoadOp loadOp = LoadOp::LOAD;
		StoreOp storeOp = StoreOp::STORE;
		ResourceState nextUsage = ResourceState::SHADER_RESOURCE;
	};

	struct RenderPassTargets
	{
		Array<RenderTargetDesc, MAX_RENDERTARGETS> rt = {};
		RenderTargetDesc ds = {};
	};

	// - Internal Resources ----------------------------------------------------------------------- //

	enum class ResourceType
	{
		UNKNOWN,
		BUFFER,
		TEXTURE,
		PIPELINE,
		COUNT,
	};
	static const char* g_ResourceTypeNames[]
	{
		"Unknown",
		"Buffer",
		"Texture",
		"Pipeline",
	};
	static_assert(NELEM(g_ResourceTypeNames) == IDX(ResourceType::COUNT));	

	template<typename T>
	class Resource
	{
		template<typename T, typename H, const uint32 SIZE> friend class HandlePool;
	public:
		Resource(ResourceType type) : m_Type(type) {}

		const ResourceType GetResourceType() { return m_Type; }
		const char* GetResourceTypeName() { return g_ResourceTypeNames[IDX(m_Type)]; }

	protected:
		ResourceType m_Type;

	private:
		Handle<T> m_Handle;
	};

#define __VAST_RESOURCE_TYPE_COMMON_DECL(className, resourceType)										\
	className() : Resource<className>(resourceType) {}													\
	static const ResourceType GetStaticResourceType() { return resourceType; }							\
	static const char* GetStaticResourceTypeName() { return g_ResourceTypeNames[IDX(resourceType)]; }

	class Buffer : public Resource<Buffer>
	{
	public:
		__VAST_RESOURCE_TYPE_COMMON_DECL(Buffer, ResourceType::BUFFER);
	};

	class Texture : public Resource<Texture>
	{
	public:
		__VAST_RESOURCE_TYPE_COMMON_DECL(Texture, ResourceType::TEXTURE);
	};

	class Pipeline : public Resource<Pipeline>
	{
	public:
		__VAST_RESOURCE_TYPE_COMMON_DECL(Pipeline, ResourceType::PIPELINE);
	};

}