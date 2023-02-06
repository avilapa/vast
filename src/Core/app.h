#pragma once

namespace vast
{
	class IApp
	{
	public:
		virtual ~IApp() = default;
		virtual void Run() = 0;
	};
}

#define VAST_DEFINE_MAIN(function)		\
int main(int argc, char** argv)			\
{										\
	function(argc, argv);				\
	return 0;							\
}

#define VAST_DEFINE_APP_MAIN(className) \
void RunApp(int argc, char** argv)		\
{										\
	className app;						\
	return app.Run();					\
}										\
VAST_DEFINE_MAIN(RunApp);