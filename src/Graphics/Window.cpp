#include "vastpch.h"
#include "Graphics/Window.h"

#ifdef VAST_PLATFORM_WINDOWS
#include "Graphics/Platform/x64/Win32_Window.h"
#else
#error "Invalid Platform: Unknown Platform"
#endif

namespace vast
{
	Ptr<Window> Window::Create(const WindowParams& params /* = WindowParams() */)
	{
		VAST_INFO("[window] Creating window.");
#ifdef VAST_PLATFORM_WINDOWS
		return MakePtr<WindowImpl_Win32>(params);
#else
		VAST_ASSERTF(0, "Invalid Platform: Unknown Platform");
		return nullptr;
#endif
	}
}
