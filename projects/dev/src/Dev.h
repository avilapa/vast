
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
	void OnGUI();

	void CreateTriangleResources();

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;

	gfx::TextureHandle m_ColorRT;
	gfx::PipelineHandle m_FullscreenPso;
	gfx::PipelineHandle m_TrianglePso;
	gfx::BufferHandle m_TriangleVtxBuf;

	uint32 m_TriangleVtxBufIdx;
	uint32 m_ColorTextureIdx;
};
