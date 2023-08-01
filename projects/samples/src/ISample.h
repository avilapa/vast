#pragma once

#include "vast.h"

using namespace vast;

class ISample
{
public:
	ISample(gfx::GraphicsContext& ctx_) : ctx(ctx_) {}
	virtual ~ISample() {}

	virtual void BeginFrame() 
	{ 
		ctx.BeginFrame(); 
	}

	virtual void EndFrame() 
	{
		vast::Profiler::EndFrame(ctx);
		ctx.EndFrame();
	}

	virtual void Update() {}
	virtual void Render() {}
	virtual void OnGUI() {}
	virtual void OnWindowResizeEvent(const WindowResizeEvent&) {}

protected:
	gfx::GraphicsContext& ctx;
};
