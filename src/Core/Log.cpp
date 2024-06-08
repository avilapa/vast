#include "vastpch.h"
#include "Core/Log.h"
#include "Core/Tracing.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace vast
{
	Ref<spdlog::logger> log::g_Logger;

	void log::Init(const char* logOutFileName)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		Ref<spdlog::sinks::sink> stdoutSink = MakeRef<spdlog::sinks::stdout_color_sink_mt>();
		stdoutSink->set_pattern("%^[%T] %n: %v%$");

		bool bTruncateFile = true; // i.e. clear file contents with every run.
		Ref<spdlog::sinks::sink> fileSink = MakeRef<spdlog::sinks::basic_file_sink_mt>(logOutFileName, bTruncateFile);
		fileSink->set_pattern("[%T] [%l] %n: %v");

		Vector<Ref<spdlog::sinks::sink>> sinks = { stdoutSink, fileSink };
		g_Logger = MakeRef<spdlog::logger>("vast", begin(sinks), end(sinks));
		spdlog::register_logger(g_Logger);

		VAST_LOG_INFO("[log] Hello there! Logger initialized successfully.");
		VAST_LOG_TRACE("[log] Created output file '{}'", logOutFileName);

		// TODO: Expose these options
		spdlog::level::level_enum logLevel = spdlog::level::trace;
		g_Logger->set_level(logLevel);
		g_Logger->flush_on(logLevel);

		VAST_LOG_TRACE("[log] Logging level set to'{}'", spdlog::level::to_string_view(logLevel));
	}
		
	void log::Stop()
	{
		spdlog::shutdown();
	}
}