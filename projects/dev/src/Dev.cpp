#include "Dev.h"

#include "shaders_shared.h"

// ---------------------------------------- TODO LIST ------------------------------------------ //
// Features coming up:
//
//	> Shader Hot Reload: allow shaders to be recompiled and reloaded live.
//		- req: Imgui Renderer (or some sort of input)
//	> Shader Precompilation: precompile shaders to avoid compiling at every start-up.
//	> Shader Visual Studio integration 1: syntax coloring for hlsl files.
//	> Shader Visual Studio integration 2: shader compilation from solution.
//		- req: Shader Precompilation
//
//	> GFX Rasterizer State.
//	> GFX Stencil State.
//	> GFX Samplers.
//	> GFX Render Pass / Render Pass Layout.
//	> GFX Compute Shaders.
//	> GFX Display List: command recording for later execution.
//	> GFX Multithreading.
//		- req: GFX Display List
//
//	> DX12 Dynamic Buffers: allow creation of non-static vertex/index buffers.
//
//	> Imgui Renderer.
//		- req: DX12 Dynamic Buffers
//
// --------------------------------------------------------------------------------------------- //

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
 	m_GraphicsContext = gfx::GraphicsContext::Create();
	gfx::GraphicsContext& ctx = *m_GraphicsContext;

	m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(ctx);

	// The vertex layout struct is declared in the shared.h file.
	Array<TriangleVtx, 3> vertexData =
	{ {
		{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
		{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
		{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
	} };

	// Create vertex buffer in CPU-writable/GPU-readable memory.
	auto vtxBufDesc = gfx::BufferDesc::Builder()
		.Size(sizeof(vertexData)).Stride(sizeof(vertexData[0]))
		.ViewFlags(gfx::BufferViewFlags::SRV)
		.IsRawAccess(true);

	m_TriangleVtxBuf = ctx.CreateBuffer(vtxBufDesc, &vertexData, sizeof(vertexData));

	// The constant buffer contains the index of the vertex buffer in the descriptor heap.
	TriangleCBV cbvData = { ctx.GetBindlessHeapIndex(m_TriangleVtxBuf) };
	m_TriangleCbv = ctx.CreateBuffer(gfx::BufferDesc::Builder()
		.Size(sizeof(TriangleCBV))
		.ViewFlags(gfx::BufferViewFlags::CBV)
		.IsRawAccess(true),
		&cbvData, sizeof(cbvData));

	// TODO: Can we defer binding RT layout to a PSO until it's used for rendering? (e.g. Separate RenderPassLayout object).
	auto pipelineDesc = gfx::PipelineDesc::Builder()
		.VS("triangle.hlsl", "VS_Main")
		.PS("triangle.hlsl", "PS_Main")
		.DepthStencil(gfx::DepthStencilState::Preset::kDisabled)
		.SetRenderTarget(ctx.GetBackBufferFormat());
	m_TrianglePipeline = ctx.CreatePipeline(pipelineDesc);

	m_TriangleCbvProxy = ctx.LookupShaderResource(m_TrianglePipeline, "ObjectConstantBuffer");
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
