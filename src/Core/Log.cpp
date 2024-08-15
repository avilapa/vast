#include "vastpch.h"
#include "Core/Log.h"
#include "Core/Tracing.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace vast
{
	static Ref<spdlog::logger> s_Logger;

	Ref<spdlog::logger>& log::GetLogger()
	{
		VAST_ASSERTF(s_Logger, "Logger not yet initialized!");
		return s_Logger;
	}

	void log::Init()
	{
		// Create debug output sink
		Ref<spdlog::sinks::sink> stdoutSink = MakeRef<spdlog::sinks::stdout_color_sink_mt>();
		stdoutSink->set_pattern("%^[%T] %n: %v%$");

		// Create and register logger object
		s_Logger = MakeRef<spdlog::logger>("vast", stdoutSink);
		spdlog::register_logger(s_Logger);

		VAST_LOG_INFO("[log] Hello there! Logger initialized successfully.");

		// TODO: Expose these options
		spdlog::level::level_enum logLevel = spdlog::level::trace;
		s_Logger->set_level(logLevel);
		s_Logger->flush_on(logLevel);
		VAST_LOG_TRACE("[log] Logging level set to'{}'", spdlog::level::to_string_view(logLevel));
	}

	void log::Stop()
	{
		spdlog::shutdown();
	}

	void log::CreateFileSink(const std::string& filePath)
	{
		VAST_PROFILE_TRACE_FUNCTION;

		bool bTruncateFile = true; // i.e. clear file contents with every run.
		Ref<spdlog::sinks::sink> fileSink = MakeRef<spdlog::sinks::basic_file_sink_mt>(filePath, bTruncateFile);
		fileSink->set_pattern("[%T] [%l] %n: %v");
		s_Logger->sinks().push_back(fileSink);
		VAST_LOG_TRACE("[log] Created output file '{}'", filePath);
	}
}