#include "vastpch.h"
#include "Core/app.h"
#include "Core/event_types.h"
#include "Graphics/window.h"

namespace vast
{

	WindowedApp::WindowedApp(int argc, char** argv)
		: m_bRunning(false)
		, m_Window(nullptr)
	{
		VAST_PROFILE_FUNCTION();

		Log::Init();
		(void)argc; (void)argv; // TODO: Process input args.
		m_Window = Window::Create();

		VAST_SUBSCRIBE_TO_EVENT(WindowCloseEvent, WindowedApp::OnWindowCloseEvent);
	}

	WindowedApp::~WindowedApp()
	{
		VAST_PROFILE_FUNCTION();

		m_Window = nullptr;
	}

	void WindowedApp::Run()
	{
		VAST_PROFILE_FUNCTION();

		m_bRunning = true;

		while (m_bRunning)
		{
			m_Window->OnUpdate();
			OnUpdate();
		}
	}

	void WindowedApp::Quit()
	{
		VAST_WARNING("Quitting application.");
		m_bRunning = false;
	}

	void WindowedApp::OnUpdate()
	{

	}

	void WindowedApp::OnWindowCloseEvent()
	{
		Quit();
	}

}