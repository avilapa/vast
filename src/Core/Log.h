#pragma once

#if (_MSC_VER >= 1929) && (__cplusplus >= 202002L) // VS2019 16.10 && C++20
// Read more: https://github.com/gabime/spdlog/pull/2170
#define SPDLOG_USE_STD_FORMAT
#endif
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
