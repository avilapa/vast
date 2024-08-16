#pragma once

#include "Core/Defines.h"
#include "Core/Assert.h"

#ifdef VAST_PLATFORM_WINDOWS
#define VAST_DEFINE_APP_MAIN(className)							\
extern int Win32_Main(int argc, char** argv, vast::IApp* app);	\
																\
int main(int argc, char** argv)									\
{																\
	static className app = {};									\
	return Win32_Main(argc, argv, &app);						\
}																
#else
#error "Unsupported platform!"
#endif

namespace vast
{

	class Window;
	class GraphicsContext;

	class IApp
	{
	public:
		IApp() = default;
		virtual ~IApp() = default;

		virtual bool Init() = 0;
		virtual void Stop() = 0;

		virtual void Update(float dt) = 0;
		virtual void Draw() = 0;

		Window& GetWindow() const 
		{	
			VAST_ASSERT(m_Window);
			return *m_Window; 
		}
		
		GraphicsContext& GetGraphicsContext() const 
		{
			VAST_ASSERT(m_GraphicsContext);
			return *m_GraphicsContext; 
		}

	public:
		Ptr<Window> m_Window;
		Ptr<GraphicsContext> m_GraphicsContext;

		bool m_bQuit = false;
	};

}
