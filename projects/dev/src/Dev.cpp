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
 	ctx.Reset();

	gfx::Texture backBuffer = ctx.GetCurrentBackBuffer();
	ctx.AddBarrier(backBuffer, gfx::ResourceState::RENDER_TARGET);
	ctx.FlushBarriers();

 	ctx.ClearRenderTarget(backBuffer, float4(0.6, 0.2, 0.9, 1.0));

	ctx.AddBarrier(backBuffer, gfx::ResourceState::PRESENT);
	ctx.FlushBarriers();

  	ctx.Submit();
 	ctx.EndFrame();
 	ctx.Present();
}