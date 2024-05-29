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

		// Initialize tracing first so that we can profile initialization, log next because most systems
		// use it to log progress and any issues. 
		trace::Init("vast-profile.json");
		VAST_PROFILE_TRACE_BEGIN("App Startup");
		log::Init("vast.log");
		profile::ui::Init();

		VAST_SUBSCRIBE_TO_EVENT("windowedapp", WindowCloseEvent, VAST_EVENT_HANDLER_CB(WindowedApp::Quit));
	}

	WindowedApp::~WindowedApp()
	{
		VAST_UNSUBSCRIBE_FROM_EVENT("windowedapp", WindowCloseEvent);
		m_GraphicsContext = nullptr;
		m_Window = nullptr;

		log::Stop();
		VAST_PROFILE_TRACE_END("App Shutdown");
		trace::Stop();
	}

	void WindowedApp::Run()
	{
		VAST_PROFILE_TRACE_END("App Startup");
		VAST_PROFILE_TRACE_BEGIN("App Loop");

		Window& window = GetWindow();

		m_bRunning = true;

		while (m_bRunning)
		{
			profile::BeginFrame();
			{
				VAST_PROFILE_TRACE_SCOPE("Update");
				VAST_PROFILE_CPU_SCOPE("Update");
				window.Update();
				Update();
			}
			{
				VAST_PROFILE_TRACE_SCOPE("Draw");
				VAST_PROFILE_CPU_SCOPE("Draw");
				Draw();
			}
			profile::EndFrame(GetGraphicsContext());
			// TODO: We should periodically call trace::Flush().
		}

		VAST_PROFILE_TRACE_END("App Loop");
		VAST_PROFILE_TRACE_BEGIN("App Shutdown");
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