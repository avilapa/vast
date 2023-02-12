#include "dev.h"

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
	m_GraphicsContext = gfx::GraphicsContext::Create();
}

Dev::~Dev()
{

}