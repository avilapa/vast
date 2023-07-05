#pragma once

#include "Core/Core.h"
#include "Core/Log.h"

#include <filesystem>

// Don't allow VAST_ASSERT to be called with too many arguments to catch missing F in compile time.
#pragma warning(error: 4002)

#if VAST_ENABLE_ASSERTS
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
#define VAST_ASSERTF(expr, fmt, ...)
#endif
