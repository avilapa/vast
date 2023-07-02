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

protected:
	gfx::GraphicsContext& m_GraphicsContext;
};
