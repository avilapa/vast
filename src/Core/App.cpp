#include "vastpch.h"
#include "Core/App.h"

#include "Core/EventTypes.h"
#include "Core/Window.h"
#include "Graphics/GraphicsContext.h"

namespace vast
{

	WindowedApp::WindowedApp(int argc, char** argv)
		: m_bRunning(false)
		, m_Window(nullptr)
		, m_GraphicsContext(nullptr)
	{
		(void)argc; (void)argv; // TODO: Process input args.

		// Initialize Trace Logger first so that we can profile initialization.
		TraceLogger::Init("vast-profile.json");
		VAST_PROFILE_TRACE_BEGIN("app", "App Startup");
		// Initialize Log next because all our systems make use of it to log progress.
		Log::Init();
#if VAST_ENABLE_PROFILING
		Profiler::Init();
#endif
		VAST_SUBSCRIBE_TO_EVENT("windowedapp", WindowCloseEvent, VAST_EVENT_HANDLER_CB(WindowedApp::Quit));
	}

	WindowedApp::~WindowedApp()
	{
		VAST_UNSUBSCRIBE_FROM_EVENT("windowedapp", WindowCloseEvent);
		m_GraphicsContext = nullptr;
		m_Window = nullptr;

		VAST_PROFILE_TRACE_END("app", "App Shutdown");
		TraceLogger::Stop();
	}

	void WindowedApp::Run()
	{
		VAST_PROFILE_TRACE_END("app", "App Startup");
		VAST_PROFILE_TRACE_BEGIN("app", "App Loop");

		Window& window = GetWindow();

		m_bRunning = true;

		while (m_bRunning)
		{
#if VAST_ENABLE_PROFILING
			Profiler::BeginFrame();
#endif
			{
				VAST_PROFILE_TRACE_SCOPE("app", "Update");
				VAST_PROFILE_CPU_SCOPE("Update");
				window.Update();
				Update();
			}
			{
				VAST_PROFILE_TRACE_SCOPE("app", "Draw");
				VAST_PROFILE_CPU_SCOPE("Draw");
				Draw();
			}
#if VAST_ENABLE_PROFILING
			Profiler::EndFrame(GetGraphicsContext());
#endif
		}

		VAST_PROFILE_TRACE_END("app", "App Loop");
		VAST_PROFILE_TRACE_BEGIN("app", "App Shutdown");
	}

	void WindowedApp::Quit()
	{
		VAST_LOG_WARNING("Quitting application.");
		m_bRunning = false;
	}

	Window& WindowedApp::GetWindow() const
	{
		VAST_ASSERTF(m_Window, "Windowed Apps must initialize 'm_Window' via vast::Window::Create().")
		return *m_Window;
	}
	
	gfx::GraphicsContext& WindowedApp::GetGraphicsContext() const
	{
		VAST_ASSERTF(m_GraphicsContext, "Windowed Apps must initialize 'm_GraphicsContext' via vast::gfx::GraphicsContext::Create().")
		return *m_GraphicsContext;
	}

}