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
		auto bufDesc = gfx::BufferDesc::Builder()
			.Size(sizeof(vertexData)).Stride(sizeof(vertexData[0]))
			.ViewFlags(gfx::BufferViewFlags::SRV)
			.IsRawAccess(true);
		m_TriangleVtxBuf = ctx.CreateBuffer(bufDesc, &vertexData, sizeof(vertexData));
	}
	{
		// The constant buffer contains the index of the vertex buffer in the descriptor heap.
		TriangleCBV cbvData = { ctx.GetBindlessHeapIndex(m_TriangleVtxBuf) };

		m_TriangleCbv = ctx.CreateBuffer(gfx::BufferDesc::Builder()
			.Size(sizeof(TriangleCBV))
			.ViewFlags(gfx::BufferViewFlags::CBV)
			.IsRawAccess(true), 
			&cbvData, sizeof(cbvData));
	}
	{
		// TODO: Can we defer binding RT layout to a PSO until it's used for rendering? (e.g. Separate RenderPassLayout object).
		auto pipelineDesc = gfx::PipelineDesc::Builder()
			.VS(L"triangle.hlsl", L"VS_Main")
			.PS(L"triangle.hlsl", L"PS_Main")
			.SetRenderTarget(gfx::Format::RGBA8_UNORM_SRGB); // TODO: This should internally query the backbuffer format.

		m_TrianglePipeline = ctx.CreatePipeline(pipelineDesc);

		m_TriangleCbvProxy = ctx.LookupShaderResource(m_TrianglePipeline, "ObjectConstantBuffer");
	}
}

Dev::~Dev()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.DestroyBuffer(m_TriangleVtxBuf);
	ctx.DestroyBuffer(m_TriangleCbv);
	ctx.DestroyPipeline(m_TrianglePipeline);
}

void Dev::OnUpdate()
{
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	ctx.BeginFrame();
	ctx.BeginRenderPass(m_TrianglePipeline);
	{
		ctx.SetShaderResource(m_TriangleCbv, m_TriangleCbvProxy);
		ctx.Draw(3);
	}
	ctx.EndRenderPass();
	ctx.EndFrame();
}
