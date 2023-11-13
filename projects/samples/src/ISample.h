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
		ctx.EndFrame();
	}

	virtual void Update() {}
	virtual void Render() {}
	virtual void OnGUI() {}
	virtual void OnWindowResizeEvent(const WindowResizeEvent&) {}
	virtual void OnReloadShadersEvent() {}

protected:
	gfx::GraphicsContext& ctx;
};
