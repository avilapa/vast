#pragma once

#define VAST_DEFINE_MAIN(function)				\
int main(int argc, char** argv)					\
{												\
	function(argc, argv);						\
	return 0;									\
}

#define VAST_DEFINE_APP_MAIN(className)			\
void RunApp(int argc, char** argv)				\
{												\
	className* app = new className(argc, argv);	\
	app->Run();									\
	delete app;									\
}												\
VAST_DEFINE_MAIN(RunApp);

namespace vast
{
	class IApp
	{
	public:
		IApp() = default;
		virtual ~IApp() = default;
		virtual void Run() = 0;
	};

	class Window;

	namespace gfx
	{
		class GraphicsContext;
	}

	class WindowedApp : public IApp
	{
	public:
		WindowedApp(int argc, char** argv);
		~WindowedApp();

		void Run() override;

		Window& GetWindow() const;
		gfx::GraphicsContext& GetGraphicsContext() const;

	protected:
		virtual void Update() = 0;
		virtual void Draw() = 0;
		void Quit();

		Ptr<Window> m_Window;
		Ptr<gfx::GraphicsContext> m_GraphicsContext;

	private:
		bool m_bRunning;
	};
}
