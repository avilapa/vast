#include "Samples.h"

#include "imgui/imgui.h"

VAST_DEFINE_APP_MAIN(Samples)

using namespace vast;

Samples::Samples(int argc, char** argv) : WindowedApp(argc, argv)
{
	auto windowSize = m_Window->GetWindowSize();

	gfx::GraphicsParams params;
	params.swapChainSize = windowSize;
	params.swapChainFormat = gfx::Format::RGBA8_UNORM;
	params.backBufferFormat = gfx::Format::RGBA8_UNORM;

	m_GraphicsContext = gfx::GraphicsContext::Create(params);
	m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(*m_GraphicsContext);
}

Samples::~Samples()
{

}

void Samples::OnUpdate()
{
	
}

void Samples::OnGUI()
{
	ImGui::ShowDemoWindow();

	if (ImGui::Begin("vast UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		
	}
	ImGui::End();
}
