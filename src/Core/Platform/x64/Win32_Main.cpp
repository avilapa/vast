#include "vastpch.h"

#include "Core/Core.h"
#include "Core/App.h"
#include "Core/EventTypes.h"

#include "Core/Platform/x64/Win32_Window.h"

#ifdef VAST_PLATFORM_WINDOWS

using namespace vast;

vast::Arg g_ProjectDir("ProjectDir");
vast::Arg g_OutputDir("OutputDir");

int Win32_Main(int argc, char** argv, IApp* app)
{
	VAST_ASSERT(app);

	// Initialize debug output logging first.
	VAST_LOGGING_ONLY(Log::Init());

	if (argc > 1)
	{
		// Process input arguments, since most systems depend on it.
		std::string argsFileName = argv[1];
		if (!Arg::Init(argsFileName))
		{
			VAST_LOGGING_ONLY(Log::Stop());
			return EXIT_FAILURE;
		}
	}
	else
	{
		VAST_LOG_WARNING("No response file found!");
	}

	std::string outputDir = "";
	g_OutputDir.Get(outputDir);
	// Initialize tracing as soon as possible so we can track systems init timings.
	VAST_TRACING_ONLY(Trace::Init(outputDir + "trace.json"));

	// - Init ------------------------------------------------------------------------------------- //
	{
		VAST_PROFILE_TRACE_SCOPE("Main Init");

		// Start logging to file.
		VAST_LOGGING_ONLY(Log::CreateFileSink(outputDir + "vast.log"));

		if (!app->Init()
			|| !VAST_VERIFYF(app->m_Window, "App must initialize 'm_Window' via vast::Window::Create().")
			|| !VAST_VERIFYF(app->m_GraphicsContext, "App must initialize 'm_GraphicsContext' via vast::gfx::GraphicsContext::Create()."))
		{
			app->Stop();
			VAST_LOGGING_ONLY(Log::Stop());
			VAST_TRACING_ONLY(Trace::Stop());

			return EXIT_FAILURE;
		}
	}

	// - Loop ------------------------------------------------------------------------------------- //
	{
		VAST_PROFILE_TRACE_SCOPE("Main Loop");

		auto QuitAppCb = [&app](const IEvent&)
		{
			VAST_LOG_WARNING("Quitting application.");
			app->m_bQuit = true;
		};
		VAST_SUBSCRIBE_TO_EVENT("main", WindowCloseEvent, QuitAppCb);
#if VAST_ENABLE_PROFILING
		auto ToggleProfilerCb = [](const IEvent&)
		{
			Profiler::ui::g_bShowProfiler = !Profiler::ui::g_bShowProfiler;
		};
		VAST_SUBSCRIBE_TO_EVENT("main", DebugActionEvent, ToggleProfilerCb);
#endif

		while (!app->m_bQuit)
		{
			VAST_PROFILING_ONLY(Profiler::BeginFrame());
			{
				VAST_PROFILE_TRACE_SCOPE("Update");
				VAST_PROFILE_CPU_SCOPE("Update");
				app->GetWindow().Update();
				app->Update(0.0f);
			}
			{
				VAST_PROFILE_TRACE_SCOPE("Draw");
				VAST_PROFILE_CPU_SCOPE("Draw");
				app->Draw();
			}
			VAST_PROFILING_ONLY(Profiler::EndFrame(app->GetGraphicsContext()));
			// TODO: We should periodically call Trace::Flush().
		}

		VAST_UNSUBSCRIBE_FROM_EVENT("main", WindowCloseEvent);
		VAST_PROFILING_ONLY(VAST_UNSUBSCRIBE_FROM_EVENT("main", DebugActionEvent));
	}

	// - Stop ------------------------------------------------------------------------------------- //
	{
		VAST_PROFILE_TRACE_BEGIN("Main Shutdown");
		app->Stop();
		VAST_ASSERTF(!app->m_Window, "Forgot to delete 'm_Window'!");
		VAST_ASSERTF(!app->m_GraphicsContext, "Forgot to delete 'm_GraphicsContext'!");
	}

	VAST_TRACING_ONLY(Trace::Stop());
	VAST_LOGGING_ONLY(Log::Stop());

	return EXIT_SUCCESS;
}

#endif // VAST_PLATFORM_WINDOWS
