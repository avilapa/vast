
#include "vast.h"

using namespace vast;

class Dev : public WindowedApp
{
public:
	Dev(int argc, char** argv);
	~Dev();

private:
	void OnUpdate() override;

	Ptr<gfx::GraphicsContext> m_GraphicsContext;

	gfx::ShaderHandle m_TriangleShaderHandles[2];
	gfx::BufferHandle m_VertexBufferHandle;
	gfx::BufferHandle m_TriangleCBVHandle;
	gfx::PipelineHandle m_PipelineHandle;

	gfx::ShaderHandle m_ImguiShaderHandles[2];

};