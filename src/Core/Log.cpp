#include "vastpch.h"
#include "Core/Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace vast
{
	Ref<spdlog::logger> Log::s_Logger;

	void Log::Init()
	{
#ifdef VAST_ENABLE_LOGGING
		VAST_PROFILE_SCOPE("log", "Log Init");
		std::string logOutputFileName = "vast.log";
		Vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logOutputFileName, true));

		logSinks[0]->set_pattern("%^[%T] %n: %v%$");
		logSinks[1]->set_pattern("[%T] [%l] %n: %v");

		s_Logger = std::make_shared<spdlog::logger>("vast", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_Logger);
		VAST_INFO("[log] Hello there! Logger initialized successfully.");
		VAST_INFO("[log] Created output file '{}'", logOutputFileName);

		// TODO: Expose these options
		spdlog::level::level_enum logLevel = spdlog::level::trace;
		s_Logger->set_level(spdlog::level::trace);
		VAST_INFO("[log] Logging level set to'{}'", spdlog::level::to_string_view(logLevel));
		s_Logger->flush_on(spdlog::level::trace);
#endif // VAST_ENABLE_LOGGING
	}	
}