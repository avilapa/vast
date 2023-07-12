#pragma once

#include "vast.h"

using namespace vast;

class ISample
{
public:
	ISample(gfx::GraphicsContext& ctx_) : ctx(ctx_) {}
	virtual ~ISample() {}

	virtual void BeginFrame() = 0;
	virtual void Render() = 0;
	virtual void EndFrame() = 0;

	virtual void Update() {}
	virtual void OnGUI() {}
	virtual void OnWindowResizeEvent(const WindowResizeEvent&) {}

protected:
	gfx::GraphicsContext& ctx;
};
