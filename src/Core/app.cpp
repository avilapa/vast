#include "vastpch.h"
#include "Core/app.h"
#include "Core/event_types.h"

namespace vast
{

	WindowedApp::WindowedApp(int argc, char** argv)
		: m_bRunning(false)
		, m_Window(nullptr)
	{
		Log::Init();
		(void)argc; (void)argv;// TODO: Process input args.
		m_Window = Window::Create();

		VAST_SUBSCRIBE_TO_EVENT(WindowCloseEvent, WindowedApp::OnWindowCloseEvent);
		VAST_SUBSCRIBE_TO_EVENT_DATA(WindowResizeEvent, WindowedApp::OnWindowResizeEvent);
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

	void WindowedApp::OnWindowCloseEvent()
	{
		Quit();
	}

	void WindowedApp::OnWindowResizeEvent(WindowResizeEvent& event)
	{
		VAST_INFO("OnWindowResizeEvent() called, to {}x{}", event.m_WindowSize.x, event.m_WindowSize.y);
	}
}