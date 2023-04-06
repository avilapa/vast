
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
	void CreateFullscreenPassResources();

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;

	gfx::TextureHandle m_ColorRT;
	gfx::ClearParams m_ClearColorRT;

	gfx::PipelineHandle m_TrianglePso;
	gfx::BufferHandle m_TriangleVtxBuf;
	gfx::BufferHandle m_TriangleCbv;
	gfx::ShaderResourceProxy m_TriangleCbvProxy;

	gfx::PipelineHandle m_FullscreenPso;
	gfx::BufferHandle m_FullscreenCbv;
	gfx::ShaderResourceProxy m_FullscreenCbvProxy;

};
