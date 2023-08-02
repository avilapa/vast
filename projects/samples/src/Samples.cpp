#include "Samples.h"

#include "ISample.h"
#include "SampleScenes/00_HelloTriangle.h"
#include "SampleScenes/01_Hello3D.h"
#include "SampleScenes/02_Instancing.h"

#include "Rendering/ImguiRenderer.h"
#include "imgui/imgui.h"

VAST_DEFINE_APP_MAIN(SamplesApp)

enum class SampleScenes
{
	HELLO_TRIANGLE,
	HELLO_3D,
	INSTANCING,
	COUNT,
};
static const char* s_SampleSceneNames[]
{
	"Hello Triangle",
	"Hello 3D",
	"Instancing",
};
static_assert(NELEM(s_SampleSceneNames) == IDX(SampleScenes::COUNT));

SamplesApp::SamplesApp(int argc, char** argv) 
	: WindowedApp(argc, argv)
	, m_GraphicsContext(nullptr)
	, m_CurrentSample(nullptr)
	, m_CurrentSampleIdx(0)
	, m_SampleInitialized(false)
{
	gfx::GraphicsParams params;
	params.swapChainSize = GetWindow().GetSize();
	params.swapChainFormat = gfx::TexFormat::RGBA8_UNORM;
	params.backBufferFormat = gfx::TexFormat::RGBA8_UNORM;

	m_GraphicsContext = gfx::GraphicsContext::Create(params);
	m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(*m_GraphicsContext);
}

SamplesApp::~SamplesApp()
{
	m_CurrentSample = nullptr;
	m_ImguiRenderer = nullptr;
	m_GraphicsContext = nullptr;
}

void SamplesApp::Update()
{
	if (!m_SampleInitialized)
	{
		if (m_CurrentSample)
		{
			m_CurrentSample = nullptr;
			// Note: Flushing the GPU is not strictly necessary, but it ensures all resources used in the
			// current scene are destroyed before loading a new scene.
			m_GraphicsContext->FlushGPU();
		}

		VAST_WARNING("[Samples] Loading sample scene '{}'", s_SampleSceneNames[m_CurrentSampleIdx]);
		GetWindow().SetName("vast - Samples : " + std::string(s_SampleSceneNames[m_CurrentSampleIdx]));

		switch (m_CurrentSampleIdx)
		{
		case IDX(SampleScenes::HELLO_TRIANGLE): m_CurrentSample = MakePtr<HelloTriangle>(*m_GraphicsContext); break;
		case IDX(SampleScenes::HELLO_3D):		m_CurrentSample = MakePtr<Hello3D>(*m_GraphicsContext); break;
		case IDX(SampleScenes::INSTANCING):		m_CurrentSample = MakePtr<Instancing>(*m_GraphicsContext); break;
		default: return;
		}

		m_SampleInitialized = true;
	}

	m_CurrentSample->Update();
}

void SamplesApp::Draw()
{
	VAST_ASSERT(m_SampleInitialized && m_CurrentSample);

	m_ImguiRenderer->BeginCommandRecording();
	m_CurrentSample->OnGUI();
	DrawSamplesEditorUI();
	vast::Profiler::OnGUI();
	m_ImguiRenderer->EndCommandRecording();

	m_CurrentSample->BeginFrame();
	VAST_PROFILE_GPU_BEGIN("GPU Frame", *m_GraphicsContext);
	{
		VAST_PROFILE_GPU_SCOPE("Sample", *m_GraphicsContext);
		m_CurrentSample->Render();
	}
	VAST_PROFILE_GPU_BEGIN("Imgui", *m_GraphicsContext);
	m_ImguiRenderer->Render();
	VAST_PROFILE_GPU_END("Imgui", *m_GraphicsContext);
	VAST_PROFILE_GPU_END("GPU Frame", *m_GraphicsContext);

	m_CurrentSample->EndFrame();

	m_Timer.Update();
	VAST_INFO("Frame time: {:.3f} ms", m_Timer.GetDeltaMilliseconds<float>());
}

void SamplesApp::DrawSamplesEditorUI()
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
