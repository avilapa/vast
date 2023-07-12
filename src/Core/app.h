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
	VAST_PROFILE_INIT("vast-profile.json");		\
	className* app = new className(argc, argv);	\
	app->Run();									\
	delete app;									\
	VAST_PROFILE_STOP();						\
}												\
VAST_DEFINE_MAIN(RunApp);

namespace vast
{
	class Window;

	class IApp
	{
	public:
		IApp() = default;
		virtual ~IApp() = default;
		virtual void Run() = 0;
	};

	class WindowedApp : public IApp
	{
	public:
		WindowedApp(int argc, char** argv);
		~WindowedApp();

		void Run() override;

	protected:
		virtual void Update() = 0;
		virtual void Draw() = 0;
		void Quit();

		Window& GetWindow() const;

	private:
		bool m_bRunning;
		Ptr<Window> m_Window;
	};
}
