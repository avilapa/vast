#include "vastpch.h"
#include "Core/Core.h"
#include "Core/Platform/x64/Win32_Window.h"

#ifdef VAST_PLATFORM_WINDOWS

using namespace vast;

static void ToggleProfilerGUI()
{
	profile::ui::g_bShowProfiler = !profile::ui::g_bShowProfiler;
}

int Win32_Main(int argc, char** argv, vast::IApp* app)
{
	(void)argc; (void)argv; // TODO: Process input args.
	
#if VAST_ENABLE_TRACING
	// Initialize tracing first so that we can profile startup.
	trace::Init("vast-profile.json");
#endif
	{
		VAST_PROFILE_TRACE_SCOPE("Main Startup");
#if VAST_ENABLE_LOGGING
		const char* outputLogFileName = "vast.log";
		log::Init(outputLogFileName);
#endif
		if (!app->Init())
		{
			return EXIT_FAILURE;
		}

		if (!VAST_VERIFYF(app->m_Window, "App must initialize 'm_Window' via vast::Window::Create()."))
		{
			return EXIT_FAILURE;
		}

		if (!VAST_VERIFYF(app->m_GraphicsContext, "App must initialize 'm_GraphicsContext' via vast::gfx::GraphicsContext::Create()."))
		{
			return EXIT_FAILURE;
		}
	}
	{
		VAST_PROFILE_TRACE_SCOPE("Main Loop");

		auto QuitAppCb = [&app](const IEvent&) 
		{ 
			VAST_LOG_WARNING("Quitting application.");
			app->m_bQuit = true; 
		};

		VAST_SUBSCRIBE_TO_EVENT("main", WindowCloseEvent, QuitAppCb);
#if VAST_ENABLE_PROFILING
		VAST_SUBSCRIBE_TO_EVENT("main", DebugActionEvent, VAST_EVENT_HANDLER_CB_STATIC(ToggleProfilerGUI));
#endif
		while (!app->m_bQuit)
		{
#if VAST_ENABLE_PROFILING
			profile::BeginFrame();
#endif
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
#if VAST_ENABLE_PROFILING
			profile::EndFrame(app->GetGraphicsContext());
#endif
			// TODO: We should periodically call trace::Flush().
		}

		VAST_UNSUBSCRIBE_FROM_EVENT("main", WindowCloseEvent);
#if VAST_ENABLE_PROFILING
		VAST_UNSUBSCRIBE_FROM_EVENT("main", DebugActionEvent);
#endif
	}
	{
		VAST_PROFILE_TRACE_SCOPE("Main Shutdown");
		app->Stop();
		VAST_ASSERTF(!app->m_Window, "Forgot to delete 'm_Window'!");
		VAST_ASSERTF(!app->m_GraphicsContext, "Forgot to delete 'm_GraphicsContext'!");

#if VAST_ENABLE_LOGGING
		log::Stop();
#endif
	}
#if VAST_ENABLE_TRACING
	trace::Stop();
#endif
	return EXIT_SUCCESS;
}

#endif // VAST_PLATFORM_WINDOWS
