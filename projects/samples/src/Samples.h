#pragma once

#include "vast.h"

using namespace vast;

class SampleBase;

class SamplesApp : public WindowedApp
{
public:
	SamplesApp(int argc, char** argv);
	~SamplesApp();

private:
	void Update() override;
	void Render() override;
	void OnGUI();

	Ptr<SampleBase> m_CurrentSample;
	uint32 m_CurrentSampleIdx;
};