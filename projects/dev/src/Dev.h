
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

	gfx::BufferHandle m_TriangleVtxBuf;
	gfx::BufferHandle m_TriangleCbv;
	gfx::PipelineHandle m_TrianglePipeline;
	gfx::ShaderResourceProxy m_TriangleCbvProxy;
};
