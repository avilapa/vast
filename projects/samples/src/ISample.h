#pragma once

#include "vast.h"

using namespace vast;

class ISample
{
public:
	ISample(GraphicsContext& ctx_) : ctx(ctx_) {}
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
	GraphicsContext& ctx;

	// Project's shader source path relative to the working directory.
	const std::string& m_SamplesShaderSourcePath = "../../projects/samples/src/Shaders/";
};
