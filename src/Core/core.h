#pragma once

#include <filesystem>

#include "Core/log.h"

// - Asserts ----------------------------------------------------------------------------------- //

#ifdef VAST_DEBUG
#define VAST_ENABLE_ASSERTS
#define VAST_DEBUGBREAK() __debugbreak()
#else
#define VAST_DEBUGBREAK()
#endif // VAST_DEBUG

// Don't allow VAST_ASSERT to be called with too many arguments.
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
				__LINE__,																\
				__VA_ARGS__);															\
			VAST_DEBUGBREAK();															\
		}																				\
	}

#define VAST_ASSERT(expr)				__VAST_ASSERT_IMPL(expr)
#define VAST_ASSERTF(expr, fmt, ...)	__VAST_ASSERT_IMPL(expr, fmt, __VA_ARGS__)
#else
#define VAST_ASSERT(expr)
#define VAST_ASSERTF(expr, fmt, ...)
#endif // VAST_ENABLE_ASSERTS

// - Utility ----------------------------------------------------------------------------------- //

// Macro expansion
#define EXP(x)		x
// Macro stringify
#define STR(x)		#x
#define XSTR(x)		STR(x)
// Macro concatenation
#define CAT(a, b)	a ## b
#define XCAT(a, b)	CAT(a, b)
// Number of elements in array (pointers not supported)
#define NELEM(x) int(sizeof(x) / sizeof((x)[0]))

#define BIT(x) (1 << x)