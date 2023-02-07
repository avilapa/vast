#pragma once

// TODO: Unable to compile this library using any version higher than C++17.
#include "spdlog/include/spdlog/spdlog.h"

// Formatting cheatsheet: https://hackingcpp.com/cpp/libs/fmt.html
#define VAST_TRACE(...)		::vast::Log::GetLogger()->trace(__VA_ARGS__)
#define VAST_INFO(...)		::vast::Log::GetLogger()->info(__VA_ARGS__)
#define VAST_WARNING(...)	::vast::Log::GetLogger()->warn(__VA_ARGS__)
#define VAST_ERROR(...)		::vast::Log::GetLogger()->error(__VA_ARGS__)
#define VAST_CRITICAL(...)	::vast::Log::GetLogger()->critical(__VA_ARGS__)

namespace vast
{
	class Log
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }

	private:
		static std::shared_ptr<spdlog::logger> s_Logger;
	};
}
