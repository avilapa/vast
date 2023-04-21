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

class SampleBase3D : public SampleBase
{
public:
	SampleBase3D(gfx::GraphicsContext& ctx);
	virtual ~SampleBase3D();

protected:
	void SetRenderTargets();
	void DrawToBackBuffer();

	gfx::TextureHandle m_ColorRT;
	gfx::TextureHandle m_DepthRT;

	gfx::PipelineHandle m_FullscreenPso;
	uint32 m_ColorTexIdx;
};
