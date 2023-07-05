#pragma once

#include "Core/Core.h"

#if (_MSC_VER >= 1929) && (__cplusplus >= 202002L) // VS2019 16.10 && C++20
// Read more: https://github.com/gabime/spdlog/pull/2170
#define SPDLOG_USE_STD_FORMAT
#endif
#include "spdlog/include/spdlog/spdlog.h"

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
#endif

namespace vast
{
	class Log
	{
	public:
		static void Init();
		inline static Ref<spdlog::logger>& GetLogger() { return s_Logger; }

	private:
		static Ref<spdlog::logger> s_Logger;
	};
}
