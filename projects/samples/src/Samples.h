#pragma once

#include "vast.h"

namespace vast::gfx
{
	class ImguiRenderer;
}

using namespace vast;

class SamplesApp final : public WindowedApp
{
public:
	SamplesApp(int argc, char** argv);
	~SamplesApp();

private:
	void Update() override;
	void Draw() override;

	void DrawSamplesEditorUI();

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;
	Ptr<class ISample> m_CurrentSample;
	uint32 m_CurrentSampleIdx;
	bool m_SampleInitialized;
	Timer m_Timer;
};