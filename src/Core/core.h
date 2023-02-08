#pragma once

#include "Core/log.h"
#include <filesystem>

// - Utility ----------------------------------------------------------------------------------- //

// Macro expansion
#define EXP(x)		x
// Macro stringify
#define STR(x)		#x
#define XSTR(x)		STR(x)
// Macro concatenation
#define CAT(a, b)	a ## b
#define XCAT(a, b)	CAT(a, b)

#define SELECT_MACRO(_1, _2, x, ...) x
// Usage: #define FOO(...) EXP(SELECT_MACRO(__VA_ARGS__, FOO_2_PARAMS, FOO_1_PARAM)(__VA_ARGS__))

// Number of elements in array (pointers not supported)
#define NELEM(x) int(sizeof(x) / sizeof((x)[0]))

#define BIT(x) (1 << x)

// - Platform ---------------------------------------------------------------------------------- //

#ifdef _WIN32
#ifdef _WIN64
#define VAST_PLATFORM_WINDOWS
#else
#error "Invalid Platform: x86 builds not supported"
#endif // _WIN64
#else
#error "Invalid Platform: Unknown Platform"
#endif

// - Asserts ----------------------------------------------------------------------------------- //

#ifdef VAST_DEBUG
#define VAST_ENABLE_ASSERTS
#define VAST_DEBUGBREAK() __debugbreak()
#else
#define VAST_DEBUGBREAK()
#endif // VAST_DEBUG

// Don't allow VAST_ASSERT to be called with too many arguments to catch missing F in compile time.
#pragma warning(error: 4002)

#ifdef VAST_ENABLE_ASSERTS
// TODO: Look into likely/unlikely for asserts.
#define __VAST_ASSERT_IMPL(expr, fmt, ...)												\
	{																					\
		if(!(expr))																		\
		{																				\
			VAST_ERROR("Assert '{}' FAILED ({}, line {}). " fmt,						\
				STR(expr),																\
				std::filesystem::path(__FILE__).filename().string().c_str(),			\
				__VA_ARGS__);															\
			VAST_DEBUGBREAK();															\
		}																				\
	}

// Note: SELECT_MACRO will not work here because VAST_ASSERTF can take any number of arguments.
// Passing __LINE__ into the macro to avoid trailing comma from __VA_ARGS__.
#define VAST_ASSERT(expr)				__VAST_ASSERT_IMPL(expr, "", __LINE__)
#define VAST_ASSERTF(expr, fmt, ...)	__VAST_ASSERT_IMPL(expr, fmt, __LINE__, __VA_ARGS__)
#else
#define VAST_ASSERT(expr)
#define VAST_ASSERTF(expr, fmt...)
#endif // VAST_ENABLE_ASSERTS
