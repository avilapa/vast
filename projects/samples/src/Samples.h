
#include "vast.h"
#include "Graphics/ImguiRenderer.h"
#include "shaders_shared.h"

using namespace vast;

class Samples : public WindowedApp
{
public:
	Samples(int argc, char** argv);
	~Samples();

private:
	void OnUpdate() override;
	void OnGUI();

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;
};
