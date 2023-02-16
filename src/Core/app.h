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

	protected:
		virtual void OnUpdate();

		void OnWindowCloseEvent();

		bool m_bRunning;
		Ptr<Window> m_Window;
	};
}

#define VAST_DEFINE_MAIN(function)				\
int main(int argc, char** argv)					\
{												\
	function(argc, argv);						\
	return 0;									\
}

#define VAST_DEFINE_APP_MAIN(className)			\
void RunApp(int argc, char** argv)				\
{												\
	VAST_PROFILE_INIT("vast-profile.json");		\
	className* app = new className(argc, argv);	\
	app->Run();									\
	delete app;									\
	VAST_PROFILE_STOP();						\
}												\
VAST_DEFINE_MAIN(RunApp);