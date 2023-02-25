#include "Dev.h"

#include "shaders_shared.h"

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
 	m_GraphicsContext = gfx::GraphicsContext::Create();
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	{
		auto vsDesc = gfx::ShaderDesc::Builder()
			.Type(gfx::ShaderType::VERTEX)
			.ShaderName(L"triangle.hlsl")
			.EntryPoint(L"VS_Main");

		auto psDesc = gfx::ShaderDesc::Builder()
			.Type(gfx::ShaderType::PIXEL)
			.ShaderName(L"triangle.hlsl")
			.EntryPoint(L"PS_Main");
		m_TriangleShaderHandles[0] = ctx.CreateShader(vsDesc);
		m_TriangleShaderHandles[1] = ctx.CreateShader(psDesc);
	}
	{
		// The vertex layout struct is declared in the shared.h file.
		Array<TriangleVtx, 3> vertexData =
		{ {
			{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
			{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
			{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
		} };

		// Create vertex buffer in CPU-writable/GPU-readable memory.
		auto bufDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(vertexData)).Stride(sizeof(vertexData[0]))
			.ViewFlags(gfx::BufferViewFlags::SRV)
			.IsRawAccess(true);
		m_VertexBufferHandle = ctx.CreateBuffer(bufDesc, &vertexData, sizeof(vertexData));
	}
	{
		// The constant buffer contains the index of the vertex buffer in the descriptor heap.
		TriangleCBV cbvData = { ctx.GetBindlessHeapIndex(m_VertexBufferHandle) };

		auto bufDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(TriangleCBV))
			.ViewFlags(gfx::BufferViewFlags::CBV)
			.IsRawAccess(true);
		m_TriangleCBVHandle = ctx.CreateBuffer(bufDesc, &cbvData, sizeof(cbvData));
	}

}

Dev::~Dev()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.DestroyShader(m_TriangleShaderHandles[0]);
	ctx.DestroyShader(m_TriangleShaderHandles[1]);
	ctx.DestroyBuffer(m_VertexBufferHandle);
	ctx.DestroyBuffer(m_TriangleCBVHandle);
}

void Dev::OnUpdate()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.BeginFrame();
	ctx.BeginRenderPass();
	ctx.EndRenderPass();
	ctx.EndFrame();
}