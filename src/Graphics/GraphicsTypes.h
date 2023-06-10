#pragma once

namespace vast::gfx
{

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

	enum class BufViewFlags
	{
		NONE = 0,
		CBV = BIT(0),
		SRV = BIT(1),
		UAV = BIT(2),
	};
	ENUM_CLASS_ALLOW_FLAGS(BufViewFlags);

	// TODO: Should CPU writable and Dynamic be exclusive? (cpu write is always dynamic, in which case dynamic would only be used for upload buffers)
	enum class BufCpuAccess
	{
		NONE,
		WRITE,
		// TODO: READBACK
	};

	enum class TexFormat
	{
		UNKNOWN,
		RG32_FLOAT,
		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,
		D32_FLOAT,
		R16_UINT,
		COUNT,
	};

	enum class TexType
	{
		UNKNOWN,
		TEXTURE_1D,
		TEXTURE_2D,
		TEXTURE_3D,
	};

	enum class TexViewFlags
	{
		NONE = 0,
		RTV = BIT(0),
		DSV = BIT(1),
		SRV = BIT(2),
		UAV = BIT(3),

		RTV_SRV = RTV | SRV,
		DSV_SRV = DSV | SRV,
	};
	ENUM_CLASS_ALLOW_FLAGS(TexViewFlags);

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

	union ClearValue
	{
		ClearValue(float4 color) : color(color) {}
		ClearValue(float depth, uint8 stencil) : ds(depth, stencil) {}

		float4 color = float4(1, 1, 1, 1);
		struct ClearDepthStencil
		{
			ClearDepthStencil(float depth, uint8 stencil) : depth(depth), stencil(stencil) {}

			float depth;
			uint8 stencil;
		} ds;
	};

// 	union TextureDimensions
// 	{
// 		uint4 dimensions = uint4(1, 1, 1, 1);
// 		struct
// 		{
// 			uint32 width;
// 			uint32 height;
// 			uint32 depthOrArraySize;
// 			uint32 mipCount;
// 		};
// 		static_assert(sizeof(uint4) == (sizeof(uint32) * 4));
// 	};

}