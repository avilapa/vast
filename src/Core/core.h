#pragma once

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

// Enum as index
#define IDX(x) static_cast<int>(x)

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

// - Logging ----------------------------------------------------------------------------------- //

#include "Core/log.h"

#define VAST_ENABLE_LOGGING 1

// Formatting cheatsheet: https://hackingcpp.com/cpp/libs/fmt.html
#if VAST_ENABLE_LOGGING
#define VAST_TRACE(...)		::vast::Log::GetLogger()->trace(__VA_ARGS__)
#define VAST_INFO(...)		::vast::Log::GetLogger()->info(__VA_ARGS__)
#define VAST_WARNING(...)	::vast::Log::GetLogger()->warn(__VA_ARGS__)
#define VAST_ERROR(...)		::vast::Log::GetLogger()->error(__VA_ARGS__)
#define VAST_CRITICAL(...)	::vast::Log::GetLogger()->critical(__VA_ARGS__)
#else
#define VAST_TRACE(...)
#define VAST_INFO(...)
#define VAST_WARNING(...)
#define VAST_ERROR(...)
#define VAST_CRITICAL(...)
#endif // VAST_LOGGING_ENABLED

// - Asserts ----------------------------------------------------------------------------------- //

#ifdef VAST_DEBUG
#define VAST_ENABLE_ASSERTS 1
#define VAST_DEBUGBREAK() __debugbreak()
#else
#define VAST_ENABLE_ASSERTS 0
#define VAST_DEBUGBREAK()
#endif // VAST_DEBUG

// Don't allow VAST_ASSERT to be called with too many arguments to catch missing F in compile time.
#pragma warning(error: 4002)

#if VAST_ENABLE_ASSERTS

#include <filesystem>

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
#endif // VAST_ENABLE_ASSERTS

// - Profiling --------------------------------------------------------------------------------- //

#define VAST_ENABLE_PROFILING 1

#if  VAST_ENABLE_PROFILING
#include "minitrace/minitrace.h"

#define VAST_PROFILE_INIT(fileName)	mtr_init(fileName);
#define VAST_PROFILE_STOP()			mtr_flush(); mtr_shutdown();

#define VAST_PROFILE_FUNCTION()		MTR_SCOPE_FUNC()
#define VAST_PROFILE_SCOPE(c, n)	MTR_SCOPE(c, n)
#define VAST_PROFILE_BEGIN(c, n)	MTR_BEGIN(c, n)
#define VAST_PROFILE_END(c, n)		MTR_END(c, n)
#else
#define VAST_PROFILE_INIT(fileName)
#define VAST_PROFILE_STOP()

#define VAST_PROFILE_FUNCTION()
#define VAST_PROFILE_SCOPE(c, n)
#define VAST_PROFILE_BEGIN(c, n)
#define VAST_PROFILE_END(c, n)
#endif // VAST_PROFILE
