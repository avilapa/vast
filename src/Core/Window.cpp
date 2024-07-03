#include "vastpch.h"
#include "Core/Window.h"

#ifdef VAST_PLATFORM_WINDOWS
#include "Core/Platform/x64/Win32_Window.h"
#else
#error "Invalid Platform: Unknown Platform"
#endif

namespace vast
{
	Ptr<Window> Window::Create()
	{
		VAST_LOG_INFO("[window] Initializing Window...");
#ifdef VAST_PLATFORM_WINDOWS
		return MakePtr<WindowImpl_Win32>();
#else
		VAST_ASSERTF(0, "Invalid Platform: Unknown Platform");
		return nullptr;
#endif
	}
}
