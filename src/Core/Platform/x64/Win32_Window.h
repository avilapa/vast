#pragma once

#include "Core/Window.h"

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
		WindowImpl_Win32();
		virtual ~WindowImpl_Win32();

		void Show() override;

		void Update() override;

		WindowHandle GetNativeHandle() const override { return static_cast<WindowHandle>(m_Handle); }
		float GetAspectRatio() const override { return float(m_WindowSize.x) / float(m_WindowSize.y); }
		uint2 GetSize() const override { return m_WindowSize; }
		std::string GetName() const override;

		void SetSize(uint2 newSize) override;
		void SetName(const std::string& name) override;

	private:
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);

		void OnWindowResize();

		uint2 GetFullWindowSize(uint2 clientSize) const;

		HWND m_Handle;
		uint2 m_WindowSize;
	};
}

#endif // VAST_PLATFORM_WINDOWS
