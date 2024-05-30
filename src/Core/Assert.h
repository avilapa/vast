#pragma once

#include "Core/Defines.h"
#include "Core/Log.h"

#include <filesystem>
#define VAST_GET_FILE_FROM_PATH std::filesystem::path(__FILE__).filename().string().c_str()

// Ensure VAST_ASSERT and VAST_ASSERTF (and VERIFY variants) are called with the right number of 
// arguments.
#pragma warning(error: 4002 4003)

#ifdef VAST_DEBUG
#define VAST_DEBUGBREAK() __debugbreak()

// TODO: Look into likely/unlikely for asserts.

#if VAST_ENABLE_LOGGING

#define __VAST_ASSERT_IMPL(expr, fmt, ...)							\
	do																\
	{																\
		if(!(expr))													\
		{															\
			VAST_LOG_CRITICAL("Assert '{}' FAILED ({}, line {}). " 	\
			fmt, STR(expr), VAST_GET_FILE_FROM_PATH, __VA_ARGS__);	\
			VAST_DEBUGBREAK();										\
		}															\
	} while(0)

#define __VAST_VERIFY_IMPL(expr, fmt, ...) ((expr) ||				\
	(VAST_LOG_CRITICAL("Verify '{}' FAILED ({}, line {}). " 		\
	fmt, STR(expr), VAST_GET_FILE_FROM_PATH, __VA_ARGS__),			\
	VAST_DEBUGBREAK(), 0))

#else

#define __VAST_ASSERT_IMPL(expr, fmt, ...)							\
	do																\
	{																\
		if(!(expr))	VAST_DEBUGBREAK();								\
	} while(0)

#define __VAST_VERIFY_IMPL(expr, fmt, ...) ((expr) || (VAST_DEBUGBREAK(), 0))

#endif // VAST_ENABLE_LOGGING

// Note: SELECT_MACRO will not work here because VAST_ASSERTF can take any number of arguments.
// Passing __LINE__ into the macro to avoid trailing comma from __VA_ARGS__.
#define VAST_ASSERT(expr)				__VAST_ASSERT_IMPL(expr, "", __LINE__)
#define VAST_ASSERTF(expr, fmt, ...)	__VAST_ASSERT_IMPL(expr, fmt, __LINE__, __VA_ARGS__)
#else
#define VAST_DEBUGBREAK()

#if VAST_ENABLE_LOGGING

#define __VAST_VERIFY_IMPL(expr, fmt, ...) ((expr) ||				\
	(VAST_LOG_CRITICAL("Verify"" '{}' FAILED ({}, line {}). " 		\
	fmt, STR(expr), VAST_GET_FILE_FROM_PATH, __VA_ARGS__), 0))

#else

#define __VAST_VERIFY_IMPL(expr, fmt, ...) (expr)

#endif // VAST_ENABLE_LOGGING

#define VAST_ASSERT(expr)
#define VAST_ASSERTF(expr, fmt, ...)
#endif

#define VAST_VERIFY(expr)				__VAST_VERIFY_IMPL(expr, "", __LINE__)
#define VAST_VERIFYF(expr, fmt, ...)	__VAST_VERIFY_IMPL(expr, fmt, __LINE__, __VA_ARGS__)
