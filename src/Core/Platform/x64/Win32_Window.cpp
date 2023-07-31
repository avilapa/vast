#include "vastpch.h"
#include "Core/Platform/x64/Win32_Window.h"
#include "Core/EventTypes.h"

#ifdef VAST_PLATFORM_WINDOWS

#include "imgui/backends/imgui_impl_win32.h" // TODO: USE_IMGUI_STOCK_IMPL_WIN32
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace vast
{
	LRESULT CALLBACK WindowImpl_Win32::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lparam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lparam))
		{
			return true;
		}

		WindowImpl_Win32* window = reinterpret_cast<WindowImpl_Win32*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		static bool bIsWindowBeingResized;

		switch (msg)
		{
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				PostQuitMessage(0);
			}
			else if (wParam == VK_SPACE)
			{
				VAST_FIRE_EVENT(DebugActionEvent);
			}
			break;
		case WM_ENTERSIZEMOVE:
			bIsWindowBeingResized = true;
			break;
		case WM_EXITSIZEMOVE:
			bIsWindowBeingResized = false;
			[[fallthrough]];
		case WM_SIZE: 
			if (!bIsWindowBeingResized && wParam != SIZE_MINIMIZED)
			{
				window->OnWindowResize();
			}
			break;
		case WM_DESTROY:
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		default:
			break;
		}

		return DefWindowProcW(hWnd, msg, wParam, lparam);
	}

	WindowImpl_Win32::WindowImpl_Win32(const WindowParams& params)
		: m_WindowSize(params.size)
		, m_WindowName(std::wstring(params.name.begin(), params.name.end()))
	{
		VAST_PROFILE_TRACE_SCOPE("window", "Create Window Impl");
		HINSTANCE hInst = GetModuleHandle(nullptr);

		const wchar_t* windowClassName = L"default";
		Register(hInst, windowClassName);
		Create(hInst, windowClassName, m_WindowName.c_str(), m_WindowSize);

		{
			VAST_PROFILE_TRACE_SCOPE("window", "Show Window");
			ShowWindow(m_Handle, SW_SHOW);
		}
		SetForegroundWindow(m_Handle);
		SetFocus(m_Handle);
		ShowCursor(true);
		VAST_INFO("[window] Window '{}' created successfully with resolution {}x{}.", params.name, params.size.x, params.size.y);
	}

	WindowImpl_Win32::~WindowImpl_Win32()
	{
		VAST_PROFILE_TRACE_SCOPE("window", "Destroy Window Impl");
		DestroyWindow(m_Handle);
		m_Handle = nullptr;
	}

	void WindowImpl_Win32::Update()
	{
		VAST_PROFILE_TRACE_SCOPE("window", "Update (Window)");
		MSG msg{ 0 };
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			VAST_FIRE_EVENT(WindowCloseEvent);
		}
	}

	void WindowImpl_Win32::SetSize(uint2 newSize)
	{
		VAST_ASSERTF(newSize.x != 0 && newSize.y != 0, "Attempted to set innvalid window size.");
		uint2 windowSize = GetFullWindowSize(newSize);
	
		// Remove WS_MAXIMIZE style
		LONG_PTR style = GetWindowLongPtr(m_Handle, GWL_STYLE);
		style &= ~WS_MAXIMIZE;
		SetWindowLongPtr(m_Handle, GWL_STYLE, style);

		::SetWindowPos(m_Handle, 0, 0, 0, windowSize.x, windowSize.y, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		
		OnWindowResize();
	}

	uint2 WindowImpl_Win32::GetSize() const
	{
		return m_WindowSize;
	}

	float WindowImpl_Win32::GetAspectRatio() const
	{
		return float(m_WindowSize.x) / float(m_WindowSize.y);
	}

	void WindowImpl_Win32::SetName(const std::string& name)
	{
		SetWindowText(m_Handle, name.c_str());
	}

	std::string WindowImpl_Win32::GetName() const
	{
		std::string name = std::string(GetWindowTextLength(m_Handle) + 1, '\0');
		GetWindowText(m_Handle, name.data(), static_cast<int>(name.size()));
		return name;
	}

	void WindowImpl_Win32::Register(HINSTANCE hInst, const wchar_t* windowClassName)
	{
		VAST_PROFILE_TRACE_SCOPE("window", "Register Window");
		WNDCLASSEXW windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = &WndProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = hInst;
		windowClass.hIcon = ::LoadIcon(hInst, NULL);
		windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = windowClassName;
		windowClass.hIconSm = ::LoadIcon(hInst, NULL);

		static ATOM atom = ::RegisterClassExW(&windowClass);
		VAST_ASSERT(atom > 0);
	}

	void WindowImpl_Win32::Create(HINSTANCE hInst, const wchar_t* windowClassName, const wchar_t* windowName, uint2 clientSize)
	{
		VAST_PROFILE_TRACE_SCOPE("window", "Create Window");
		uint2 windowSize = GetFullWindowSize(clientSize);
		uint2 screenSize = uint2(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));

		// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
		uint2 screenCenterPos = hlslpp::max(uint2(0), uint2((screenSize.x - windowSize.x) / 2, (screenSize.y - windowSize.y) / 2));

		m_Handle = ::CreateWindowExW(NULL, windowClassName, windowName, WS_OVERLAPPEDWINDOW, 
			screenCenterPos.x, screenCenterPos.y, windowSize.x, windowSize.y, 
			NULL, NULL, hInst, nullptr);
		VAST_ASSERTF(m_Handle, "Failed to create window.");

		// Pass a pointer to the Window to the WndProc function. This can also be done via WM_CREATE.
		SetWindowLongPtr(m_Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	}

	void WindowImpl_Win32::OnWindowResize()
	{
		VAST_PROFILE_TRACE_SCOPE("window", "Resize Window");
		RECT windowRect = {};
		::GetClientRect(m_Handle, &windowRect);
		uint2 newSize = uint2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);

		if (newSize.x != m_WindowSize.x || newSize.y != m_WindowSize.y)
		{
			VAST_WARNING("[window] Resizing window to {}, {} (was {}, {})", newSize.x, newSize.y, m_WindowSize.x, m_WindowSize.y);
			m_WindowSize = newSize;
			WindowResizeEvent event(m_WindowSize);
			VAST_FIRE_EVENT(WindowResizeEvent, event);
		}
	}

	uint2 WindowImpl_Win32::GetFullWindowSize(uint2 clientSize) const
	{
		RECT windowRect = { 0, 0, static_cast<LONG>(clientSize.x), static_cast<LONG>(clientSize.y) };
		::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		return uint2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}

}

#endif // VAST_PLATFORM_WINDOWS
