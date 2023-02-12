
#include "vast.h"

using namespace vast;

class Dev : public WindowedApp
{
public:
	Dev(int argc, char** argv);
	~Dev();

private:
	std::unique_ptr<gfx::GraphicsContext> m_GraphicsContext;
};