
#include "vast.h"
#include "Graphics/ImguiRenderer.h"
#include "shaders_shared.h"

using namespace vast;

class Dev : public WindowedApp
{
public:
	Dev(int argc, char** argv);
	~Dev();

private:
	void OnUpdate() override;
	void OnGUI();

	void CreateTriangleResources(const gfx::RenderPassLayout& pass);
	void CreateCubeResources();
	void CreateSphereResources();

	Ptr<gfx::GraphicsContext> m_GraphicsContext;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;

	gfx::TextureHandle m_ColorRT;
	gfx::TextureHandle m_DepthRT;

	gfx::PipelineHandle m_FullscreenPso;
	uint32 m_ColorTexIdx;

	gfx::PipelineHandle m_TrianglePso;
	gfx::BufferHandle m_TriangleVtxBuf;
	uint32 m_TriangleVtxBufIdx;

	gfx::PipelineHandle m_MeshPso;
	gfx::ShaderResourceProxy m_MeshCbvBufProxy;

	gfx::BufferHandle m_CubeVtxBuf;
	gfx::BufferHandle m_CubeCbvBuf;	
	gfx::TextureHandle m_CubeColorTex;
	MeshCB m_CubeCB;

	gfx::BufferHandle m_SphereVtxBuf;
	gfx::BufferHandle m_SphereIdxBuf;
	gfx::BufferHandle m_SphereCbvBuf;
	gfx::TextureHandle m_SphereColorTex;
	MeshCB m_SphereCB;
};
