#pragma once

#include "Graphics/Window.h"

#ifdef VAST_PLATFORM_WINDOWS

// Note: Reduces the size of windows.h by excluding less commonly used parts of the API
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace vast
{
	class WindowImpl_Win32 : public Window
	{
	public:
		WindowImpl_Win32(const WindowParams& params);
		virtual ~WindowImpl_Win32();

		void OnUpdate() override;

		uint2 GetWindowSize() const override;

	private:
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);

		void OnWindowResize();

		void Register(HINSTANCE hInst, const wchar_t* windowClassName);
		void Create(HINSTANCE hInst, const wchar_t* windowClassName, const wchar_t* windowName, uint2 windowSize);

		HWND m_Handle;
		uint2 m_WindowSize;
		std::wstring m_WindowName;
	};
}

#endif // VAST_PLATFORM_WINDOWS
