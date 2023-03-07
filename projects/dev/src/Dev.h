
#include "vast.h"
#include "Graphics/ImguiRenderer.h"

using namespace vast;

class Dev : public WindowedApp
{
public:
	Dev(int argc, char** argv);
	~Dev();

private:
	void OnUpdate() override;

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;

	gfx::BufferHandle m_TriangleVtxBuf;
	gfx::BufferHandle m_TriangleCbv;
	gfx::PipelineHandle m_TrianglePipeline;
	gfx::ShaderResourceProxy m_TriangleCbvProxy;
};
