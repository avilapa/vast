
#include "vast.h"

using namespace vast;

class Dev : public WindowedApp
{
public:
	Dev(int argc, char** argv);
	~Dev();

private:
	Ptr<gfx::GraphicsContext> m_GraphicsContext;
};