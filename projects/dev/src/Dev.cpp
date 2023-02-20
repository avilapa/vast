#include "Dev.h"

#include "shaders_shared.h"

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
	m_GraphicsContext = gfx::GraphicsContext::Create();
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	{
		// The vertex layout struct is declared in the shared.h file.
		Array<TriangleVtx, 3> vertexData =
		{ {
			{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
			{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
			{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
		} };

		// Create vertex buffer in CPU-writable/GPU-readable memory.
		gfx::BufferDesc bufDesc;
		bufDesc.size = sizeof(vertexData);
		bufDesc.stride = sizeof(vertexData[0]);
		bufDesc.viewFlags = gfx::BufferViewFlags::SRV;
		bufDesc.isRawAccess = true;

		m_VertexBufferHandle = ctx.CreateBuffer(bufDesc, &vertexData, sizeof(vertexData));
	}
	{
		// The constant buffer contains the index of the vertex buffer in the descriptor heap.
		TriangleCBV cbvData = { ctx.GetBindlessHeapIndex(m_VertexBufferHandle) };

		gfx::BufferDesc bufDesc;
		bufDesc.size = sizeof(TriangleCBV);
		bufDesc.viewFlags = gfx::BufferViewFlags::CBV;
		bufDesc.isRawAccess = true;

		m_TriangleCBVHandle = ctx.CreateBuffer(bufDesc, &cbvData, sizeof(cbvData));
	}

}

Dev::~Dev()
{

}

void Dev::OnUpdate()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.BeginFrame();
	ctx.BeginRenderPass();
	ctx.EndRenderPass();
	ctx.EndFrame();
}