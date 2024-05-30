#pragma once

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

	namespace gfx
	{
		class GraphicsContext;
	}

	class IApp
	{
	public:
		IApp() = default;
		virtual ~IApp() = default;

		virtual bool Init() = 0;
		virtual void Stop() = 0;

		virtual void Update(float dt) = 0;
		virtual void Draw() = 0;

		bool m_bQuit = false;

		Window& GetWindow() const 
		{	
			return *m_Window; 
		}
		
		gfx::GraphicsContext& GetGraphicsContext() const 
		{ 
			return *m_GraphicsContext; 
		}

	protected:
		Ptr<Window> m_Window;
		Ptr<gfx::GraphicsContext> m_GraphicsContext;
	};

}
