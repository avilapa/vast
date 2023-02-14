#pragma once

// TODO: Unable to compile this library using any version higher than C++17.
#include "spdlog/include/spdlog/spdlog.h"

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
