#pragma once

#include "Graphics/Handles.h"
#include "Graphics/GraphicsTypes.h"

namespace vast
{
	// - Resource Handles ------------------------------------------------------------------------- //

	class Buffer {};
	class Texture {};
	class Pipeline {};

	using BufferHandle = Handle<Buffer>;
	using TextureHandle = Handle<Texture>;
	using PipelineHandle = Handle<Pipeline>;

	// - Resource Descriptors --------------------------------------------------------------------- //

	struct BufferDesc
	{
		uint32 size = 0;
		uint32 stride = 0;
		BufViewFlags viewFlags = BufViewFlags::NONE;
		ResourceUsage usage = ResourceUsage::DEFAULT;
		bool bBindless = false;
	};

	BufferDesc AllocVertexBufferDesc(uint32 size, uint32 stride, bool bBindless = true, ResourceUsage usage = ResourceUsage::DEFAULT);
	BufferDesc AllocIndexBufferDesc(uint32 numIndices, IndexBufFormat format = IndexBufFormat::R16_UINT, ResourceUsage usage = ResourceUsage::DEFAULT);
	BufferDesc AllocCbvBufferDesc(uint32 size, ResourceUsage usage = ResourceUsage::UPLOAD);
	BufferDesc AllocStructuredBufferDesc(uint32 size, uint32 stride, ResourceUsage usage = ResourceUsage::DEFAULT);

	static constexpr uint32 CONSTANT_BUFFER_ALIGNMENT = 256;

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

	TextureDesc AllocRenderTargetDesc(TexFormat format, uint2 dimensions, float4 clear = DEFAULT_CLEAR_COLOR_VALUE);
	TextureDesc AllocDepthStencilTargetDesc(TexFormat format, uint2 dimensions, ClearDepthStencil clear = { DEFAULT_CLEAR_DEPTH_VALUE, 0 });

	struct ShaderDesc
	{
		ShaderType type = ShaderType::UNKNOWN;
		std::string filePath = VAST_SHADERS_SOURCE_PATH;
		std::string shaderName = "";
		std::string entryPoint = "";
	};

	ShaderDesc AllocComputeShaderDesc(const char* shaderName, const char* filePath = VAST_SHADERS_SOURCE_PATH, const char* entryPoint = "CS_Main");
	ShaderDesc AllocVertexShaderDesc(const char* shaderName, const char* filePath = VAST_SHADERS_SOURCE_PATH, const char* entryPoint = "VS_Main");
	ShaderDesc AllocPixelShaderDesc(const char* shaderName, const char* filePath = VAST_SHADERS_SOURCE_PATH, const char* entryPoint = "PS_Main");

	struct DepthStencilState
	{
		bool depthEnable = false;
		bool depthWrite = false;
		CompareFunc depthFunc = CompareFunc::NONE;
		// TODO: Stencil state

		struct Preset;
	};

	struct DepthStencilState::Preset
	{
		static constexpr DepthStencilState kDisabled		{ false, false,	CompareFunc::NONE };
		static constexpr DepthStencilState kEnabled			{ true,	 true,	GetFixedCompareFunc(CompareFunc::LESS_EQUAL) };
		static constexpr DepthStencilState kEnabledReadOnly	{ true,	 false, GetFixedCompareFunc(CompareFunc::LESS_EQUAL) };
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
		DepthStencilState depthStencilState = DepthStencilState::Preset::kEnabled;
		RasterizerState rasterizerState = {};
		RenderPassLayout renderPassLayout = {};
	};

	struct RenderTargetDesc
	{
		TextureHandle h;
		LoadOp loadOp = LoadOp::LOAD;
		StoreOp storeOp = StoreOp::STORE;
		ResourceState nextUsage = ResourceState::NONE;
	};

	struct RenderPassDesc
	{
		Array<RenderTargetDesc, MAX_RENDERTARGETS> rt = {};
		RenderTargetDesc ds = {};
	};

}