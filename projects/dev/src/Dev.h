
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
	void CreateMeshResources();

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;

	gfx::TextureHandle m_ColorRT;
	gfx::TextureHandle m_DepthRT;
	uint32 m_ColorTexIdx;

	gfx::PipelineHandle m_FullscreenPso;

	gfx::PipelineHandle m_TrianglePso;
	gfx::BufferHandle m_TriangleVtxBuf;
	uint32 m_TriangleVtxBufIdx;

	gfx::PipelineHandle m_MeshPso;
	gfx::BufferHandle m_MeshVtxBuf;
	gfx::BufferHandle m_MeshCbvBuf;
	gfx::TextureHandle m_MeshColorTex;
	gfx::ShaderResourceProxy m_MeshCbvBufProxy;
};
