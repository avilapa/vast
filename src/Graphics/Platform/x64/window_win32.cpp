#include "vastpch.h"
#include "Graphics/Platform/x64/window_win32.h"

#include "Core/event_types.h"

#ifdef VAST_PLATFORM_WINDOWS

// TODO: Imgui
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace vast
{
	LRESULT CALLBACK WindowImpl_Win32::WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
	{
		// TODO: Imgui
// 		if (ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam))
// 		{
// 			return true;
// 		}

		WindowImpl_Win32* window = reinterpret_cast<WindowImpl_Win32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		static bool bIsWindowBeingResized;

		switch (umessage)
		{
		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
			{
				PostQuitMessage(0);
			}
			break;
		case WM_ENTERSIZEMOVE:
			bIsWindowBeingResized = true;
			break;
		case WM_EXITSIZEMOVE:
			bIsWindowBeingResized = false;
			[[fallthrough]];
		case WM_SIZE: 
			if (!bIsWindowBeingResized)
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

		return DefWindowProcW(hwnd, umessage, wparam, lparam);
	}

	WindowImpl_Win32::WindowImpl_Win32(const WindowParams& params)
		: m_WindowSize(params.size)
		, m_WindowName(std::wstring(params.name.begin(), params.name.end()))
	{
		VAST_PROFILE_SCOPE("Window", "WindowImpl_Win32::WindowImpl_Win32");

		HINSTANCE hInst = GetModuleHandle(nullptr);

		const wchar_t* windowClassName = L"default";
		Register(hInst, windowClassName);
		Create(hInst, windowClassName, m_WindowName.c_str(), m_WindowSize);

		ShowWindow(m_Handle, SW_SHOW);
		SetForegroundWindow(m_Handle);
		SetFocus(m_Handle);
		ShowCursor(true);
		VAST_INFO("[window] Window '{}' created successfully with resolution {}x{}.", params.name, params.size.x, params.size.y);
	}

	WindowImpl_Win32::~WindowImpl_Win32()
	{
		VAST_PROFILE_SCOPE("Window", "WindowImpl_Win32::~WindowImpl_Win32");

		DestroyWindow(m_Handle);
		m_Handle = nullptr;
	}

	void WindowImpl_Win32::OnUpdate()
	{
		VAST_PROFILE_SCOPE("Window", "WindowImpl_Win32::OnUpdate");

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

	void WindowImpl_Win32::OnWindowResize()
	{
		RECT windowRect = {};
		::GetClientRect(m_Handle, &windowRect);

		m_WindowSize.x = windowRect.right - windowRect.left;
		m_WindowSize.y = windowRect.bottom - windowRect.top;

		WindowResizeEvent event(m_WindowSize);
		VAST_FIRE_EVENT(WindowResizeEvent, event);
	}

	uint2 WindowImpl_Win32::GetWindowSize() const
	{
		return m_WindowSize;
	}

	void WindowImpl_Win32::Register(HINSTANCE hInst, const wchar_t* windowClassName)
	{
		VAST_PROFILE_SCOPE("Window", "WindowImpl_Win32::Register");

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

	void WindowImpl_Win32::Create(HINSTANCE hInst, const wchar_t* windowClassName, const wchar_t* windowName, uint2 windowSize)
	{
		VAST_PROFILE_SCOPE("Window", "WindowImpl_Win32::Create");

		int screenW = ::GetSystemMetrics(SM_CXSCREEN);
		int screenH = ::GetSystemMetrics(SM_CYSCREEN);

		RECT windowRect = { 0, 0, static_cast<LONG>(windowSize.x), static_cast<LONG>(windowSize.y) };
		::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		int windowW = windowRect.right - windowRect.left;
		int windowH = windowRect.bottom - windowRect.top;

		// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
		int windowX = std::max<int>(0, (screenW - windowW) / 2);
		int windowY = std::max<int>(0, (screenH - windowH) / 2);

		m_Handle = ::CreateWindowExW(NULL, windowClassName, windowName, WS_OVERLAPPEDWINDOW, windowX, windowY, windowW, windowH, NULL, NULL, hInst, nullptr);
		VAST_ASSERTF(m_Handle, "Failed to create window");

		// Pass a pointer to the Window to the WndProc function. This can also be done via WM_CREATE.
		SetWindowLongPtr(m_Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	}

}

#endif // VAST_PLATFORM_WINDOWS
