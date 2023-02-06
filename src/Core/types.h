#pragma once

#include <stdint.h>

#define HLSLPP_FEATURE_TRANSFORM
#include "hlslpp/include/hlsl++_vector_int.h"
#include "hlslpp/include/hlsl++_vector_uint.h"
#include "hlslpp/include/hlsl++_vector_float.h"
#include "hlslpp/include/hlsl++_matrix_float.h"

namespace vast
{

	// - Shader shared types ---------------------------------------------------------------------- //

	// Note: Shader shared types (s_) are needed when using hlslpp types due to these being aligned to
	// 16 bytes for SIMD reasons. Read more: https://github.com/redorav/hlslpp/issues/53
	template<typename T, unsigned N>
	class ShaderSharedType
	{
	public:
		template<class... Args>
		ShaderSharedType(Args... args) : v{ args... }
		{
			const size_t n = sizeof...(Args);
			static_assert(n == N, "Invalid number of arguments for constructor of vector type.");
		}
		T v[N];
	};

	// - Int Types -------------------------------------------------------------------------------- //

	using int8 = int8_t;
	using int16 = int16_t;
	using int32 = int32_t;
	using int64 = int64_t;

	// Vectors
	using int2 = hlslpp::int2;
	using int3 = hlslpp::int3;
	using int4 = hlslpp::int4;

	// - Uint Types ------------------------------------------------------------------------------- //

	using uint8 = uint8_t;
	using uint16 = uint16_t;
	using uint32 = uint32_t;
	using uint64 = uint64_t;

	// Vectors
	using uint2 = hlslpp::uint2;
	using uint3 = hlslpp::uint3;
	using uint4 = hlslpp::uint4;

	// - Float Types ------------------------------------------------------------------------------ //

	// Vectors								// Shader shared types
	using float2 = hlslpp::float2;			using s_float2 = ShaderSharedType<float, 2>;
	using float3 = hlslpp::float3;			using s_float3 = ShaderSharedType<float, 3>;
	using float4 = hlslpp::float4;

	// Matrices
	using float2x2 = hlslpp::float2x2;
	using float3x3 = hlslpp::float3x3;
	using float4x4 = hlslpp::float4x4;

	// -------------------------------------------------------------------------------------------- //

}