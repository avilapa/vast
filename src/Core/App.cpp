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
		VAST_PROFILE_FUNCTION();
		(void)argc; (void)argv; // TODO: Process input args.

		Log::Init();

		m_Window = Window::Create();
		VAST_SUBSCRIBE_TO_EVENT(WindowCloseEvent, VAST_EVENT_CALLBACK(WindowedApp::Quit));

		gfx::GraphicsParams params;
		params.swapChainSize = m_Window->GetSize();
		params.swapChainFormat = gfx::Format::RGBA8_UNORM;
		params.backBufferFormat = gfx::Format::RGBA8_UNORM;

		m_GraphicsContext = gfx::GraphicsContext::Create(params);
		m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(*m_GraphicsContext);
	}

	WindowedApp::~WindowedApp()
	{
		VAST_PROFILE_FUNCTION();

		m_ImguiRenderer = nullptr;
		m_GraphicsContext = nullptr;
		m_Window = nullptr;
	}

	void WindowedApp::Run()
	{
		VAST_PROFILE_FUNCTION();

		m_bRunning = true;

		while (m_bRunning)
		{
			m_GraphicsContext->BeginFrame();
			m_ImguiRenderer->BeginFrame();

			m_Window->Update();
			Update();
			Render();

			m_ImguiRenderer->EndFrame();
			m_GraphicsContext->EndFrame();
		}
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