#pragma once

#include "vast.h"

namespace vast::gfx
{
	class ImguiRenderer;
}

using namespace vast;

class SamplesApp final : public IApp
{
public:
	bool Init() override;
	void Stop() override;
	void Update(float dt) override;
	void Draw() override;

	void DrawSamplesEditorUI();

	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;
	Ptr<class ISample> m_CurrentSample;
	uint32 m_CurrentSampleIdx;
	bool m_SampleInitialized;
};