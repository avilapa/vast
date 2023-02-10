#pragma once

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
	class WindowResizeEvent;

	class WindowedApp : public IApp
	{
	public:
		WindowedApp(int argc, char** argv);
		~WindowedApp();

		void Run() override;
		void Quit();

	private:
		void OnWindowCloseEvent();
		void OnWindowResizeEvent(WindowResizeEvent& event);

		bool m_bRunning;
		std::unique_ptr<Window> m_Window;
	};
}

#define VAST_DEFINE_MAIN(function)			\
int main(int argc, char** argv)				\
{											\
	function(argc, argv);					\
	return 0;								\
}

#define VAST_DEFINE_APP_MAIN(className)		\
void RunApp(int argc, char** argv)			\
{											\
	className app = className(argc, argv);	\
	return app.Run();						\
}											\
VAST_DEFINE_MAIN(RunApp);