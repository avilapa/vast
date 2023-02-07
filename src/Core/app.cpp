#include "vastpch.h"
#include "Core/app.h"
#include "Core/event.h"

namespace vast
{

	WindowedApp::WindowedApp(int argc, char** argv)
		: m_bRunning(false)
		, m_Window(nullptr)
	{
		Log::Init();
		// TODO: Process input args.
		m_Window = Window::Create();
		VAST_SUBSCRIBE_TO_EVENT(WindowCloseEvent, VAST_EVENT_HANDLER(WindowedApp::Quit));
	}

	WindowedApp::~WindowedApp()
	{
		m_Window = nullptr;
	}

	void WindowedApp::Run()
	{
		m_bRunning = true;

		while (m_bRunning)
		{
			m_Window->OnUpdate();
		}
	}

	void WindowedApp::Quit()
	{
		VAST_WARNING("Quitting application.");
		m_bRunning = false;
	}
}