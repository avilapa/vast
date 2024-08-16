#pragma once

#include "Core/Defines.h"
#include "Core/Types.h"

#if (_MSC_VER >= 1929) && (__cplusplus >= 202002L) // VS2019 16.10 && C++20
// Read more: https://github.com/gabime/spdlog/pull/2170
#define SPDLOG_USE_STD_FORMAT
#endif
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include "spdlog/include/spdlog/spdlog.h"

// Formatting cheatsheet: https://hackingcpp.com/cpp/libs/fmt.html
#if VAST_ENABLE_LOGGING
#define VAST_LOG_TRACE(...)		::vast::Log::GetLogger()->log(spdlog::level::trace,		__VA_ARGS__)
#define VAST_LOG_INFO(...)		::vast::Log::GetLogger()->log(spdlog::level::info,		__VA_ARGS__)
#define VAST_LOG_WARNING(...)	::vast::Log::GetLogger()->log(spdlog::level::warn,		__VA_ARGS__)
#define VAST_LOG_ERROR(...)		::vast::Log::GetLogger()->log(spdlog::level::err,		__VA_ARGS__)
#define VAST_LOG_CRITICAL(...)	::vast::Log::GetLogger()->log(spdlog::level::critical,	__VA_ARGS__)
#else
#define VAST_LOG_TRACE(...)
#define VAST_LOG_INFO(...)
#define VAST_LOG_WARNING(...)
#define VAST_LOG_ERROR(...)
#define VAST_LOG_CRITICAL(...)
#endif

namespace vast
{
	namespace Log
	{
		void Init();
		void Stop();

		void CreateFileSink(const std::string& filePath);

		Ref<spdlog::logger>& GetLogger();
	};
}
