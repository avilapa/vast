#include "vastpch.h"
#include "Core/App.h"

#include "Core/EventTypes.h"
#include "Core/Window.h"

namespace vast
{

	WindowedApp::WindowedApp(int argc, char** argv)
		: m_bRunning(false)
		, m_Window(nullptr)
	{
		VAST_PROFILE_TRACE_BEGIN("app", "App Startup");
		(void)argc; (void)argv; // TODO: Process input args.

		Log::Init();

		m_Window = Window::Create();
		VAST_SUBSCRIBE_TO_EVENT("windowedapp", WindowCloseEvent, VAST_EVENT_HANDLER_CB(WindowedApp::Quit));
	}

	WindowedApp::~WindowedApp()
	{
		m_Window = nullptr;
		VAST_PROFILE_TRACE_END("app", "App Shutdown");
	}

	void WindowedApp::Run()
	{
		VAST_PROFILE_TRACE_END("app", "App Startup");
		VAST_PROFILE_TRACE_BEGIN("app", "App Loop");

		m_bRunning = true;

		while (m_bRunning)
		{
			{
				VAST_PROFILE_TRACE_SCOPE("app", "Update");
				m_Window->Update();
				Update();
			}
			{
				VAST_PROFILE_TRACE_SCOPE("app", "Draw");
				Draw();
			}
		}

		VAST_PROFILE_TRACE_END("app", "App Loop");
		VAST_PROFILE_TRACE_BEGIN("app", "App Shutdown");
	}

	void WindowedApp::Quit()
	{
		VAST_WARNING("Quitting application.");
		m_bRunning = false;
	}

	Window& WindowedApp::GetWindow() const
	{
		return *m_Window;
	}

}