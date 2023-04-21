#include "Samples.h"

#include "SampleBase.h"
#include "SampleScenes/01_HelloTriangle.h"
#include "SampleScenes/02_Hello3D.h"

#include "Graphics/ImguiRenderer.h"

#include "shaders_shared.h" // TODO: TEMP for MeshCB

#include "imgui/imgui.h"

VAST_DEFINE_APP_MAIN(SamplesApp)

using namespace vast;
using namespace vast::gfx;

SamplesApp::SamplesApp(int argc, char** argv) 
	: WindowedApp(argc, argv)
	, m_CurrentSample(nullptr)
	, m_CurrentSampleIdx(2)
{
	auto windowSize = GetWindow().GetSize();

	gfx::GraphicsParams params;
	params.swapChainSize = windowSize;
	params.swapChainFormat = gfx::Format::RGBA8_UNORM;
	params.backBufferFormat = gfx::Format::RGBA8_UNORM;
}

SamplesApp::~SamplesApp()
{

}

void SamplesApp::Update()
{
	if (!m_CurrentSample)
	{
		switch (m_CurrentSampleIdx)
		{
// 		case 1: m_CurrentSample = MakePtr<HelloTriangle>(GetGraphicsContext()); break;
// 		case 2: m_CurrentSample = MakePtr<Hello3D>(GetGraphicsContext()); break;
		default: return;
		}
	}

	m_CurrentSample->Update();

	m_CurrentSample->Draw();
	m_CurrentSample->OnGUI();

	OnGUI();
}

void SamplesApp::Render()
{

}

void SamplesApp::OnGUI()
{
	ImGui::ShowDemoWindow();

	if (ImGui::Begin("vast UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		
	}
	ImGui::End();
}
