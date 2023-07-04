#include "vastpch.h"
#include "Core/App.h"

#include "Core/EventTypes.h"

#include "Graphics/Window.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/ImguiRenderer.h"

namespace vast
{

	WindowedApp::WindowedApp(int argc, char** argv)
		: m_bRunning(false)
		, m_Window(nullptr)
		, m_GraphicsContext(nullptr)
		, m_ImguiRenderer(nullptr)
	{
		VAST_PROFILE_BEGIN("app", "App Startup");
		(void)argc; (void)argv; // TODO: Process input args.

		Log::Init();

		m_Window = Window::Create();
		VAST_SUBSCRIBE_TO_EVENT("windowedapp", WindowCloseEvent, VAST_EVENT_HANDLER_CB(WindowedApp::Quit));

		gfx::GraphicsParams params;
		params.swapChainSize = m_Window->GetSize();
		params.swapChainFormat = gfx::TexFormat::RGBA8_UNORM;
		params.backBufferFormat = gfx::TexFormat::RGBA8_UNORM;

		m_GraphicsContext = gfx::GraphicsContext::Create(params);
		m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(*m_GraphicsContext);
	}

	WindowedApp::~WindowedApp()
	{
		m_ImguiRenderer = nullptr;
		m_GraphicsContext = nullptr;
		m_Window = nullptr;
		VAST_PROFILE_END("app", "App Shutdown");
	}

	void WindowedApp::Run()
	{
		VAST_PROFILE_END("app", "App Startup");
		VAST_PROFILE_BEGIN("app", "App Loop");

		m_bRunning = true;

		while (m_bRunning)
		{
			VAST_PROFILE_BEGIN("app", "Begin Frame");
			m_GraphicsContext->BeginFrame();
			m_ImguiRenderer->BeginFrame();
			VAST_PROFILE_END("app", "Begin Frame");

			VAST_PROFILE_BEGIN("app", "Update");
			m_Window->Update();
			Update();
			VAST_PROFILE_END("app", "Update");

			VAST_PROFILE_BEGIN("app", "Render");
			Render();
			VAST_PROFILE_END("app", "Render");
			
			VAST_PROFILE_BEGIN("App", "End Frame");
			m_GraphicsContext->RenderOutputToBackBuffer();
			m_ImguiRenderer->EndFrame();
			m_GraphicsContext->EndFrame();
			VAST_PROFILE_END("app", "End Frame");
		}

		VAST_PROFILE_END("app", "App Loop");
		VAST_PROFILE_BEGIN("app", "App Shutdown");
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

	gfx::GraphicsContext& WindowedApp::GetGraphicsContext() const
	{
		return *m_GraphicsContext;
	}

}