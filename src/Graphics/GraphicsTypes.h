#pragma once

namespace vast::gfx
{
	// - Graphics constants -------------------------------------------------------------------- //

	constexpr uint32 NUM_FRAMES_IN_FLIGHT = 2;

	// TODO: Set sensible defaults
	constexpr uint32 NUM_TEXTURES = 512;
	constexpr uint32 NUM_BUFFERS = 512;
	constexpr uint32 NUM_PIPELINES = 64;
	constexpr uint32 NUM_TIMESTAMP_QUERIES = 256;

	constexpr bool ENABLE_VSYNC = true;
	constexpr bool ALLOW_TEARING = false;

	static const char* VAST_DEFAULT_SHADERS_SOURCE_PATH = "../../src/Shaders/";

	// - Graphics Enums ------------------------------------------------------------------------ //

	enum class GPUAdapterPreferenceCriteria
	{
		MAX_PERF,
		MAX_VRAM,
		MIN_POWER,
	};
	static const char* g_GPUAdapterPreferenceCriteriaNames[]
	{
		"Highest Performance",
		"Highest GPU Memory",
		"Lowest Power Consumption",
	};
	static_assert(NELEM(g_GPUAdapterPreferenceCriteriaNames) == (CountBits(IDX(GPUAdapterPreferenceCriteria::MIN_POWER)) + 1));


	enum class BufViewFlags
	{
		NONE = 0,
		CBV = BIT(0),
		SRV = BIT(1),
		UAV = BIT(2),
	};
	ENUM_CLASS_ALLOW_FLAGS(BufViewFlags);

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

	enum class IndexBufFormat
	{
		UNKNOWN,
		R16_UINT,
		R32_UINT,
	};

	enum class TexFormat
	{
		UNKNOWN,

		RGBA32_FLOAT,
		RGBA16_FLOAT,
		RG32_FLOAT,
		RG16_FLOAT,

		RGBA8_UNORM,
		RGBA8_UNORM_SRGB,

		D16_UNORM,
		D32_FLOAT,
		D32_FLOAT_S8X24_UINT,
		D24_UNORM_S8_UINT,

		R16_UINT,
		COUNT,
	};

	constexpr bool IsTexFormatDepth(TexFormat format)
	{
		switch (format)
		{
		case TexFormat::D16_UNORM:
		case TexFormat::D32_FLOAT:
		case TexFormat::D32_FLOAT_S8X24_UINT:
		case TexFormat::D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	constexpr bool IsTexFormatStencil(TexFormat format)
	{
		switch (format)
		{
		case TexFormat::D32_FLOAT_S8X24_UINT:
		case TexFormat::D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	enum class TexType
	{
		UNKNOWN,
		TEXTURE_1D,
		TEXTURE_2D,
		TEXTURE_3D,
	};

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

	constexpr CompareFunc GetFixedCompareFunc(const CompareFunc c, bool bReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z)
	{
		if (bReverseZ)
		{
			switch (c)
			{
			case CompareFunc::LESS:				return CompareFunc::GREATER;
			case CompareFunc::LESS_EQUAL:		return CompareFunc::GREATER_EQUAL;
			case CompareFunc::GREATER:			return CompareFunc::LESS;
			case CompareFunc::GREATER_EQUAL:	return CompareFunc::LESS_EQUAL;
			default: break;
			}
		}
		return c;
	}

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

	enum class ResourceState
	{
		// Common resource states
		NONE,
		PIXEL_SHADER_RESOURCE		= BIT(0),
		NON_PIXEL_SHADER_RESOURCE	= BIT(1),
		UNORDERED_ACCESS			= BIT(2),
		// TODO: Copy src/dst states
		// Texture specific states
		RENDER_TARGET				= BIT(3),
		DEPTH_WRITE					= BIT(4),
		DEPTH_READ					= BIT(5),
		// Buffer specific states
		VERTEX_BUFFER				= BIT(6),
		INDEX_BUFFER				= BIT(7),
		CONSTANT_BUFFER				= BIT(8),
	};
	ENUM_CLASS_ALLOW_FLAGS(ResourceState);
	static const char* g_ResourceStateNames[]
	{
		"None/Present",
		"Pixel Shader Resource",
		"Non Pixel Shader Resource",
		"Unordered Access",

		"Render Target",
		"Depth Write",
		"Depth Read",

		"Vertex Buffer",
		"Index Buffer",
		"Constant Buffer",
	};
	static_assert(NELEM(g_ResourceStateNames) == (CountBits(IDX(ResourceState::CONSTANT_BUFFER)) + 1));

	enum class ResourceUsage
	{
		DEFAULT,	// CPU no access, GPU read/write
		UPLOAD,		// CPU write, GPU read
		READBACK,	// CPU read, GPU write
		// TODO: GPU_UPLOAD support when it hits mainstream support on the Agility SDK
	};

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

	// - Clear Value ------------------------------------------------------------------------------ //

#define CLEAR_DEPTH_VALUE_STANDARD	1.0f
#define CLEAR_DEPTH_VALUE_REVERSE_Z	0.0f
#if VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z
#define DEFAULT_CLEAR_DEPTH_VALUE	CLEAR_DEPTH_VALUE_REVERSE_Z
#else
#define DEFAULT_CLEAR_DEPTH_VALUE	CLEAR_DEPTH_VALUE_STANDARD
#endif

#define DEFAULT_CLEAR_COLOR_VALUE	float4(0.0f, 0.0f, 0.0f, 1.0f)

	struct ClearDepthStencil
	{
		float depth = DEFAULT_CLEAR_DEPTH_VALUE;
		uint8 stencil = 0;
	};

	union ClearValue
	{
		float4 color = DEFAULT_CLEAR_COLOR_VALUE;
		ClearDepthStencil ds;
	};

}