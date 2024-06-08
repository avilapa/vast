#pragma once

#include "Core/Defines.h"

#include <iostream>
#include <format>

// TODO: Move this include to a .cpp to avoid including filesystem everywhere
#include <filesystem>
#define VAST_GET_FILE_FROM_PATH std::filesystem::path(__FILE__).filename().string().c_str()

template <typename... Args>
static void LogFatalError(const std::string_view fmt, Args&&... args)
{
	std::cerr << "\033[37m" << "\033[41m" << "\033[1m"; // Set white bold text and red background.
	std::cerr << "FATAL ERROR: " << std::vformat(fmt, std::make_format_args(args...)) << std::endl;
	std::cerr << "\033[0m"; // Reset format
}

// Ensure VAST_ASSERT and VAST_ASSERTF (and VERIFY variants) are called with the right number of 
// arguments.
#pragma warning(error: 4002 4003)

#if VAST_ENABLE_ASSERTS

// TODO: Look into likely/unlikely for asserts.
#define __VAST_ASSERT_IMPL(expr, fmt, ...)							\
	do																\
	{																\
		if(!(expr))													\
		{															\
			LogFatalError("Assert '{}' FAILED ({}, line {}). " 		\
			fmt, STR(expr), VAST_GET_FILE_FROM_PATH, __VA_ARGS__);	\
			VAST_DEBUGBREAK();										\
		}															\
	} while(0)

#define __VAST_VERIFY_IMPL(expr, fmt, ...) ((expr) ||				\
	(LogFatalError("Verify '{}' FAILED ({}, line {}). " 			\
	fmt, STR(expr), VAST_GET_FILE_FROM_PATH, __VA_ARGS__),			\
	VAST_DEBUGBREAK(), 0))

#else

#define __VAST_ASSERT_IMPL(expr, fmt, ...)

#define __VAST_VERIFY_IMPL(expr, fmt, ...) ((expr) ||				\
	(LogFatalError("Verify"" '{}' FAILED ({}, line {}). " 			\
	fmt, STR(expr), VAST_GET_FILE_FROM_PATH, __VA_ARGS__), 0))

#endif // VAST_ENABLE_ASSERTS

// Note: SELECT_MACRO will not work here because VAST_ASSERTF can take any number of arguments.
// Passing __LINE__ into the macro to avoid trailing comma from __VA_ARGS__.
#define VAST_ASSERT(expr)				__VAST_ASSERT_IMPL(expr, "", __LINE__)
#define VAST_ASSERTF(expr, fmt, ...)	__VAST_ASSERT_IMPL(expr, fmt, __LINE__, __VA_ARGS__)

#define VAST_VERIFY(expr)				__VAST_VERIFY_IMPL(expr, "", __LINE__)
#define VAST_VERIFYF(expr, fmt, ...)	__VAST_VERIFY_IMPL(expr, fmt, __LINE__, __VA_ARGS__)
