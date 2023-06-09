#pragma once

namespace vast::gfx
{

	// - Graphics Types --------------------------------------------------------------------------- //

	enum class SamplerState
	{
		LINEAR_WRAP = 0,
		LINEAR_CLAMP,
		POINT_CLAMP,
		COUNT
	};

	static const char* g_SamplerNames[]
	{
		"LinearWrapSampler",
		"LinearClampSampler",
		"PointClampSampler",
	};
	static_assert(NELEM(g_SamplerNames) == IDX(SamplerState::COUNT));

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

	enum class FillMode
	{
		SOLID,
		WIREFRAME,
	};

	enum class CullMode
	{
		NONE,
		FRONT,
		BACK,
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

	enum class LoadOp
	{
		LOAD,
		CLEAR,
		DISCARD
	};

	enum class StoreOp
	{
		STORE,
		DISCARD
	};

	// TODO: These should be flags, and could be more specific, but they should cover the basics for now
	enum class ResourceState
	{
		NONE,
		SHADER_RESOURCE,
		RENDER_TARGET,
		DEPTH_WRITE,
		DEPTH_READ,
		PRESENT,
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
		Builder& SetSize(uint32 size) { desc.size = size; return *this; }
		Builder& SetStride(uint32 stride) { desc.stride = stride; return *this; }
		Builder& SetViewFlags(BufferViewFlags viewFlags) { desc.viewFlags = viewFlags; return *this; }
		Builder& SetCpuAccess(BufferCpuAccess cpuAccess) { desc.cpuAccess = cpuAccess; return *this; }
		Builder& SetUsage(ResourceUsage usage) { desc.usage = usage; return *this; }
		Builder& SetIsRawAccess(bool isRawAccess) { desc.isRawAccess = isRawAccess; return *this; }

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
		float4 clear = float4(1);

		struct Builder;
	};

	struct TextureDesc::Builder
	{
		Builder& SetType(TextureType type) { desc.type = type; return *this; }
		Builder& SetFormat(Format format) { desc.format = format; return *this; }
		Builder& SetWidth(uint32 width) { desc.width = width; return *this; }
		Builder& SetHeight(uint32 height) { desc.height = height; return *this; }
		Builder& SetDepthOrArraySize(uint32 depthOrArraySize) { desc.depthOrArraySize = depthOrArraySize; return *this; }
		Builder& SetMipCount(uint32 mipCount) { desc.mipCount = mipCount; return *this; }
		Builder& SetViewFlags(TextureViewFlags viewFlags) { desc.viewFlags = viewFlags; return *this; }
		Builder& SetRenderTargetClearColor(float4 color) { desc.clear = color; return *this; } // RTV only
		Builder& SetDepthClearValue(float depth) { desc.clear = depth; return *this; }	// DSV only

		operator TextureDesc() { return desc; }
		TextureDesc desc;
	};

	struct ShaderDesc
	{
		ShaderType type = ShaderType::UNKNOWN;
		std::string shaderName;
		std::string entryPoint;
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

	struct RenderTargetDesc
	{
		Format format = Format::UNKNOWN;
		LoadOp loadOp = LoadOp::LOAD;
		StoreOp storeOp = StoreOp::STORE;
		ResourceState nextUsage = ResourceState::SHADER_RESOURCE;
		BlendState bs = BlendState::Preset::kDisabled;
	};

	struct RenderPassLayout
	{
		static constexpr uint32 MAX_RENDERTARGETS = 8;

		Array<RenderTargetDesc, MAX_RENDERTARGETS> renderTargets;
		RenderTargetDesc depthStencilTarget;
	};

	struct PipelineDesc
	{
		ShaderDesc vs;
		ShaderDesc ps;
		RenderPassLayout renderPassLayout;
		DepthStencilState depthStencilState;
		RasterizerState rasterizerState;

		struct Builder;
	};

	struct PipelineDesc::Builder
	{
		Builder& SetVertexShader(const std::string& fileName, const std::string& entryPoint) { desc.vs = ShaderDesc{ ShaderType::VERTEX, fileName, entryPoint }; return *this; }
 		Builder& SetPixelShader(const std::string& fileName, const std::string& entryPoint) { desc.ps = ShaderDesc{ ShaderType::PIXEL,  fileName, entryPoint }; return *this; }
		Builder& SetDepthStencilState(DepthStencilState ds) { desc.depthStencilState = ds; return *this; }
		Builder& SetRasterizerState(RasterizerState rs) { desc.rasterizerState = rs; return *this; }
		Builder& SetRenderPassLayout(RenderPassLayout pass) { desc.renderPassLayout = pass; return *this; }

		operator PipelineDesc() { return desc; }
		PipelineDesc desc;
	};
}