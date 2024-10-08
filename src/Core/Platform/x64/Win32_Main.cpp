#include "vastpch.h"
#include "Core/Core.h"
#include "Core/App.h"
#include "Core/EventTypes.h"

#include "Core/Platform/x64/Win32_Window.h"

#ifdef VAST_PLATFORM_WINDOWS

using namespace vast;

Arg g_ProjectDir("ProjectDir", std::string());
Arg g_OutputDir("OutputDir", std::string());

int Win32_Main(int argc, char** argv, IApp* app)
{
	VAST_ASSERT(app);

	// Initialize debug output logging first.
	VAST_LOGGING_ONLY(Log::Init());

	// Process input arguments next, since most systems depend on it.
	if (argc > 1)
	{
		std::string argsFileName = argv[1];
		if (!Arg::ParseArgsFile(argsFileName))
		{
			VAST_LOGGING_ONLY(Log::Stop());
			return EXIT_FAILURE;
		}
	}
	else
	{
		VAST_LOG_WARNING("No response file found!");
	}

	std::string outputDir;
	g_OutputDir.Get(outputDir);

	// Initialize tracing as soon as possible so we can track systems init timings.
	VAST_TRACING_ONLY(Trace::Init(outputDir + "trace.json"));
	// Start logging to file.
	VAST_LOGGING_ONLY(Log::CreateFileSink(outputDir + "vast.log"));

	// - Init ------------------------------------------------------------------------------------- //
	{
		VAST_PROFILE_TRACE_SCOPE("Main Init");

		if (!app->Init()
			|| !VAST_VERIFYF(app->m_Window, "App must initialize 'm_Window' via vast::Window::Create().")
			|| !VAST_VERIFYF(app->m_GraphicsContext, "App must initialize 'm_GraphicsContext' via vast::GraphicsContext::Create()."))
		{
			app->Stop();
			VAST_LOGGING_ONLY(Log::Stop());
			VAST_TRACING_ONLY(Trace::Stop());

			return EXIT_FAILURE;
		}

		app->m_Window->Show();
	}

	// - Loop ------------------------------------------------------------------------------------- //
	{
		VAST_PROFILE_TRACE_SCOPE("Main Loop");

		auto&& quitAppCb = VAST_EVENT_HANDLER_EXPR_CAPTURE(VAST_LOG_WARNING("Quitting application."); app->m_bQuit = true, &app);
		Event::Subscribe<WindowCloseEvent>("main", quitAppCb);
#if VAST_ENABLE_PROFILING
		auto&& profCb = VAST_EVENT_HANDLER_EXPR_STATIC(Profiler::ui::g_bShowProfiler = !Profiler::ui::g_bShowProfiler);
		Event::Subscribe<DebugActionEvent>("main", profCb);
#endif
		while (!app->m_bQuit)
		{
			VAST_PROFILING_ONLY(Profiler::BeginFrame());
;			{
				VAST_PROFILE_TRACE_SCOPE("Update");
				VAST_PROFILE_CPU_SCOPE("Update");
				app->m_Window->Update();
				app->Update(1.0f); // TODO: Implement proper delta time
			}
			{
				VAST_PROFILE_TRACE_SCOPE("Draw");
				VAST_PROFILE_CPU_SCOPE("Draw");
				app->Draw();
			}
			VAST_PROFILING_ONLY(Profiler::EndFrame(*app->m_GraphicsContext));
			// TODO: We should periodically call Trace::Flush().
		}

		Event::Unsubscribe<WindowCloseEvent>("main");
		VAST_PROFILING_ONLY(Event::Unsubscribe<DebugActionEvent>("main"));
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
