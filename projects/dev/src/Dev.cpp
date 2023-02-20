#include "Dev.h"

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
	m_GraphicsContext = gfx::GraphicsContext::Create();
}

Dev::~Dev()
{

}

void Dev::OnUpdate()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.BeginFrame();
	ctx.BeginRenderPass();
	ctx.EndRenderPass();
	ctx.EndFrame();
}