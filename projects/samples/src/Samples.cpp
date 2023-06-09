#include "Samples.h"

#include "SampleBase.h"
#include "SampleScenes/00_HelloTriangle.h"
#include "SampleScenes/01_Hello3D.h"

#include "imgui/imgui.h"

VAST_DEFINE_APP_MAIN(SamplesApp)

enum class SampleScenes
{
	HELLO_TRIANGLE,
	HELLO_3D,
	COUNT,
};
static const char* s_SampleSceneNames[]
{
	"Hello Triangle",
	"Hello 3D",
};
static_assert(NELEM(s_SampleSceneNames) == IDX(SampleScenes::COUNT));

SamplesApp::SamplesApp(int argc, char** argv) 
	: WindowedApp(argc, argv)
	, m_CurrentSample(nullptr)
	, m_CurrentSampleIdx(0)
	, m_SampleInitialized(false)
{
}

SamplesApp::~SamplesApp()
{
	m_CurrentSample = nullptr;
}

void SamplesApp::Update()
{
	if (!m_SampleInitialized)
	{
		gfx::GraphicsContext& ctx = GetGraphicsContext();

		if (m_CurrentSample)
		{
			m_CurrentSample = nullptr;
			// Flushing the GPU is not strictly necessary, but it ensures all resources used in the 
			// current scene are destroyed before loading a new scene.
			ctx.FlushGPU();
		}

		VAST_WARNING("[Samples] Loading sample scene '{}'", s_SampleSceneNames[m_CurrentSampleIdx]);

		switch (m_CurrentSampleIdx)
		{
		case IDX(SampleScenes::HELLO_TRIANGLE): m_CurrentSample = MakePtr<HelloTriangle>(ctx); break;
		case IDX(SampleScenes::HELLO_3D): m_CurrentSample = MakePtr<Hello3D>(ctx); break;
		default: return;
		}

		m_SampleInitialized = true;
	}

	m_CurrentSample->Update();
}

void SamplesApp::Render()
{
	VAST_ASSERT(m_SampleInitialized && m_CurrentSample);

	m_CurrentSample->Draw();
	m_CurrentSample->OnGUI();
	OnGUI();
}

void SamplesApp::OnGUI()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Load Sample..."))
		{
			for (uint32 i = 0; i < NELEM(s_SampleSceneNames); ++i)
			{
				if (ImGui::MenuItem(s_SampleSceneNames[i]))
				{
					m_CurrentSampleIdx = i;
					m_SampleInitialized = false;
				}
			}

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}
