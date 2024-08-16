#include "Samples.h"

#include "ISample.h"
#include "SampleScenes/00_HelloTriangle.h"
#include "SampleScenes/01_Hello3D.h"
#include "SampleScenes/02_Instancing.h"
#include "SampleScenes/03_Textures.h"

#include "Rendering/ImguiRenderer.h"
#include "Rendering/Imgui.h"

VAST_DEFINE_APP_MAIN(SamplesApp)

enum class SampleScenes
{
	HELLO_TRIANGLE,
	HELLO_3D,
	INSTANCING,
	TEXTURES,
	COUNT,
};
static const char* s_SampleSceneNames[]
{
	"00 - Hello Triangle",
	"01 - Hello 3D",
	"02 - Instancing",
	"03 - Textures",
};
static_assert(NELEM(s_SampleSceneNames) == IDX(SampleScenes::COUNT));

bool SamplesApp::Init()
{
	m_CurrentSample = nullptr;
	m_CurrentSampleIdx = 0;
	m_SampleInitialized = false;

	m_Window = Window::Create();

	GraphicsParams params;
	params.swapChainSize = GetWindow().GetSize();
	params.swapChainFormat = TexFormat::RGBA8_UNORM;
	params.backBufferFormat = TexFormat::RGBA8_UNORM;
	m_GraphicsContext = MakePtr<GraphicsContext>(params);

	m_ImguiRenderer = MakePtr<ImguiRenderer>(*m_GraphicsContext);

	return true;
}

void SamplesApp::Stop()
{
	m_CurrentSample = nullptr;
	m_ImguiRenderer = nullptr;
	m_GraphicsContext = nullptr;
	m_Window = nullptr;
}

void SamplesApp::Update(float dt)
{
	if (!m_SampleInitialized)
	{
		GraphicsContext& ctx = GetGraphicsContext();

		if (m_CurrentSample)
		{
			m_CurrentSample = nullptr;
			// Note: Flushing the GPU is not strictly necessary, but it ensures all resources used in the
			// current scene are destroyed before loading a new scene.
			m_GraphicsContext->FlushGPU();
			Profiler::FlushProfiles();
		}

		VAST_LOG_WARNING("[Samples] Loading sample scene '{}'", s_SampleSceneNames[m_CurrentSampleIdx]);
		GetWindow().SetName("vast - Samples : " + std::string(s_SampleSceneNames[m_CurrentSampleIdx]));

		switch (m_CurrentSampleIdx)
		{
		case IDX(SampleScenes::HELLO_TRIANGLE): m_CurrentSample = MakePtr<HelloTriangle>(ctx); break;
		case IDX(SampleScenes::HELLO_3D):		m_CurrentSample = MakePtr<Hello3D>(ctx); break;
		case IDX(SampleScenes::INSTANCING):		m_CurrentSample = MakePtr<Instancing>(ctx); break;
		case IDX(SampleScenes::TEXTURES):		m_CurrentSample = MakePtr<Textures>(ctx); break;
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
	vast::Profiler::ui::OnGUI();
	m_ImguiRenderer->EndCommandRecording();

	m_CurrentSample->BeginFrame();
	{
		VAST_PROFILE_GPU_SCOPE("Sample", GetGraphicsContext());
		m_CurrentSample->Render();
	}
	{
		VAST_PROFILE_GPU_SCOPE("Imgui", GetGraphicsContext());
		m_ImguiRenderer->Render();
	}
	m_CurrentSample->EndFrame();
}

void SamplesApp::DrawSamplesEditorUI()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Load Sample"))
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

		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Show Profiler"))
			{
				VAST_FIRE_EVENT(DebugActionEvent);
			}
			
			if (ImGui::MenuItem("Reload Shaders"))
			{
				ReloadShadersEvent e(GetGraphicsContext());
				VAST_FIRE_EVENT(ReloadShadersEvent, e);
			}
			ImGui::EndMenu();
		}

		ImGui::SameLine(ImGui::GetWindowSize().x - Profiler::ui::GetTextMinimalLength() - ImGui::GetStyle().ItemSpacing.x);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetTextLineHeightWithSpacing() - ImGui::GetTextLineHeight()) / 2);
		Profiler::ui::DrawTextMinimal();

		ImGui::EndMainMenuBar();
	}
}
