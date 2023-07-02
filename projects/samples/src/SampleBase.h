#pragma once

#include "vast.h"

using namespace vast;

class SampleBase
{
public:
	SampleBase(gfx::GraphicsContext& ctx);
	virtual ~SampleBase();

	virtual void Update();
	virtual void Draw();
	virtual void OnGUI();
	virtual void OnWindowResizeEvent(const WindowResizeEvent& event);

protected:
	gfx::GraphicsContext& m_GraphicsContext;
};
