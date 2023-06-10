#pragma once

#include "GraphicsTypes.h"

namespace vast::gfx
{
	// - Resource Descriptors --------------------------------------------------------------------- //

	// TODO: Provide functions to more easily build common configurations of Desc objects.
	struct BufferDesc
	{
		uint32 size				= 0;
		uint32 stride			= 0;
		BufViewFlags viewFlags	= BufViewFlags::NONE;
		BufCpuAccess cpuAccess	= BufCpuAccess::WRITE;
		ResourceUsage usage		= ResourceUsage::STATIC;
		bool isRawAccess		= false; // TODO: This refers to using ByteAddressBuffer to read the buffer
	};

	struct TextureDesc
	{
		TexType type			= TexType::TEXTURE_2D;
		TexFormat format		= TexFormat::RGBA8_UNORM_SRGB;
		uint32 width			= 1;
		uint32 height			= 1;
		uint32 depthOrArraySize = 1;
		uint32 mipCount			= 1;
		TexViewFlags viewFlags	= TexViewFlags::NONE;
		ClearValue clear		= ClearValue(float4(1.0f, 1.0f, 1.0f, 1.0f));
	};

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
		TexFormat format = TexFormat::UNKNOWN;
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
		ShaderDesc vs						= {};
		ShaderDesc ps						= {};
		RenderPassLayout renderPassLayout	= {};
		DepthStencilState depthStencilState = {};
		RasterizerState rasterizerState		= {};
	};

}