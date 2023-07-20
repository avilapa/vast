#include "Dev.h"

#include "imgui/imgui.h"

// ---------------------------------------- TODO LIST ------------------------------------------ //
// Features coming up:
//
//	> Shader Precompilation: precompile shaders to avoid compiling at every start-up.
//	> Shader Visual Studio integration 2: shader compilation from solution.
//		- req: Shader Precompilation
//
//	> GFX Stencil State.
//	> GFX Compute Shaders.
//	> GFX Display List: command recording for later execution.
//	> GFX Multithreading.
//		- req: GFX Display List
//	> GFX Simple Raytracing.
//		- req: Camera Object
//	> GFX Deferred Rendering.
//	> GFX Line Rendering.
//  > GFX Per Frame Resources
//  > GFX Per Pass Resources
//  > GFX Pipeline State Caching
//
//	> Object Loading from file (.obj).
//	> Scene Loading (.gltf, .usd?).
//	> Delta Time.
//	> Camera Object / Movement.
//
// --------------------------------------------------------------------------------------------- //

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;
using namespace vast::gfx;

struct Vtx2fPos3fColor
{
	s_float2 pos;
	s_float3 col;
};

static Array<Vtx2fPos3fColor, 3> s_TriangleVertexData =
{ {
	{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
	{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f }},
	{{  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
} };
static bool s_UpdateTriangle = false;
static bool s_ReloadTriangle = false;

static Array<Vtx3fPos3fNormal2fUv, 36> s_CubeVertexData =
{ {
	{{ 1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f }, { 0.0f, 0.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
	{{-1.0f,-1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
	{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
	{{-1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
	{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
	{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
	{{ 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
	{{-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
	{{ 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
	{{ 1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 1.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
	{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
	{{-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 1.0f }},
	{{ 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 1.0f, 0.0f }},
	{{-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f }, { 0.0f, 0.0f }},
} };
static bool s_ReloadMesh = false;

static Vector<Vtx3fPos3fNormal2fUv> s_SphereVertexData;
static Vector<uint16> s_SphereIndexData;

void ConstructUVSphere(const float radius, const uint32 vCount, const uint32 hCount, Vector<Vtx3fPos3fNormal2fUv>& vtx, Vector<uint16>& idx)
{
	// Generate vertices
	float invR = 1.0f / radius;
	float vStep = float(PI) / vCount;
	float hStep = 2.0f * float(PI) / hCount;

	for (uint32 v = 0; v < vCount + 1; ++v)
	{
		float vAngle = float(PI) * 0.5f - float(v) * vStep;
		float xz = radius * cos(vAngle);
		float y = radius * sin(vAngle);
		for (uint32 h = 0; h < hCount + 1; ++h)
		{
			float hAngle = h * hStep;
			float x = xz * cos(hAngle);
			float z = xz * sin(hAngle);

			Vtx3fPos3fNormal2fUv i;
			i.pos = s_float3(x, y, z);
			i.normal = s_float3(x * invR, y * invR, z * invR);
			i.uv = s_float2(float(h) / hCount, float(v) / vCount);
			vtx.push_back(i);
		}
	}
	Vtx3fPos3fNormal2fUv i;
	i.pos = s_float3(0.0f, -1.0f, 0.0f);
	vtx.push_back(i);

	// Generate indices
	uint32 k1, k2;
	for (uint32 v = 0; v < vCount; ++v)
	{
		k1 = v * (hCount + 1);
		k2 = k1 + hCount + 1;
		for (uint32 h = 0; h < hCount + 1; ++h, ++k1, ++k2)
		{
			if (v != 0)
			{
				idx.push_back(static_cast<uint16>(k1));
				idx.push_back(static_cast<uint16>(k2));
				idx.push_back(static_cast<uint16>(k1 + 1));
			}

			if (v != (vCount - 1))
			{
				idx.push_back(static_cast<uint16>(k1 + 1));
				idx.push_back(static_cast<uint16>(k2));
				idx.push_back(static_cast<uint16>(k2 + 1));
			}
		}
	}
}

//

class SimpleRenderer
{
public:
	SimpleRenderer(gfx::GraphicsContext& ctx_)
		: ctx(ctx_)
		, m_ImguiRenderer(nullptr)
	{
		VAST_SUBSCRIBE_TO_EVENT("simplerenderer", WindowResizeEvent, VAST_EVENT_HANDLER_CB(SimpleRenderer::OnWindowResizeEvent, WindowResizeEvent));

		m_ImguiRenderer = MakePtr<gfx::ImguiRenderer>(ctx);

		uint2 backBufferSize = ctx.GetBackBufferSize();
		float4 clearColor = float4(0.1f, 0.2f, 0.4f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, backBufferSize, clearColor));
		m_DepthRT = ctx.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, backBufferSize));

		m_FullscreenPso = ctx.CreatePipeline(PipelineDesc{
			.vs = {.type = ShaderType::VERTEX, .shaderName = "fullscreen.hlsl", .entryPoint = "VS_Main"},
			.ps = {.type = ShaderType::PIXEL,  .shaderName = "fullscreen.hlsl", .entryPoint = "PS_Main"},
			.depthStencilState = DepthStencilState::Preset::kDisabled,
			.renderPassLayout = {.rtFormats = { ctx.GetBackBufferFormat() } },
			});

		m_FrameCB.cameraPos = { -3.0f, 3.0f, -8.0f };
		m_FrameCB.viewProjMatrix = ComputeViewProjectionMatrix();

		BufferDesc cbvBufDesc = AllocCbvBufferDesc(sizeof(SimpleRenderer_PerFrame));
		m_FrameCbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_FrameCB, sizeof(SimpleRenderer_PerFrame));
	}

	~SimpleRenderer()
	{
		ctx.DestroyPipeline(m_FullscreenPso);
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT);
		ctx.DestroyBuffer(m_FrameCbvBuf);
	}

	void BeginFrame()
	{
		m_ImguiRenderer->BeginCommandRecording();

		ctx.BeginFrame();
		ctx.UpdateBuffer(m_FrameCbvBuf, &m_FrameCB, sizeof(SimpleRenderer_PerFrame));
	}

	void EndFrame()
	{
		// Note: We can discard instead of clear the contents of the back buffer as long as the
		// first time we draw to the target every frame we do a full-screen pass.
		ctx.BeginRenderPassToBackBuffer(m_FullscreenPso, LoadOp::DISCARD, StoreOp::STORE);
		{
			uint32 srvIndex = ctx.GetBindlessIndex(m_ColorRT);
			ctx.SetPushConstants(&srvIndex, sizeof(uint32));
			ctx.DrawFullscreenTriangle();
		}
		ctx.EndRenderPass();

		m_ImguiRenderer->EndCommandRecording();
		m_ImguiRenderer->Render();
		ctx.EndFrame();
	}

	TextureHandle GetColorRT() const { return m_ColorRT; }
	TextureHandle GetDepthRT() const { return m_DepthRT; }
	BufferHandle GetFrameCBV() const { return m_FrameCbvBuf; }
	SimpleRenderer_PerFrame& GetPerFrameConstantBuffer() { return m_FrameCB; }

private:
	void OnWindowResizeEvent(const WindowResizeEvent& event)
	{
		ctx.FlushGPU();
		ctx.DestroyTexture(m_ColorRT);
		ctx.DestroyTexture(m_DepthRT);
		float4 clearColor = float4(0.1f, 0.2f, 0.4f, 1.0f);
		m_ColorRT = ctx.CreateTexture(AllocRenderTargetDesc(TexFormat::RGBA8_UNORM, event.m_WindowSize, clearColor));
		m_DepthRT = ctx.CreateTexture(AllocDepthStencilTargetDesc(TexFormat::D32_FLOAT, event.m_WindowSize));

		m_FrameCB.viewProjMatrix = ComputeViewProjectionMatrix();
	}

	float4x4 ComputeViewProjectionMatrix()
	{
		float4x4 viewMatrix = float4x4::look_at({ -3.0f, 3.0f, -8.0f }, float3(0), float3(0, 1, 0));

		const uint2 screenSize = ctx.GetBackBufferSize();
		const float fieldOfView = float(PI) / 4.0f;
		const float aspectRatio = (float)screenSize.x / (float)screenSize.y;
		const float zNear = 0.001f;
		const float zFar = 1000.0f;
		hlslpp::frustum frustum = hlslpp::frustum::field_of_view_x(fieldOfView, aspectRatio, zNear, zFar);
		float4x4 projMatrix = float4x4::perspective(hlslpp::projection(frustum, hlslpp::zclip::t::zero));

		return hlslpp::mul(viewMatrix, projMatrix);
	}

	gfx::GraphicsContext& ctx;
	Ptr<gfx::ImguiRenderer> m_ImguiRenderer;

	TextureHandle m_ColorRT;
	TextureHandle m_DepthRT;

	PipelineHandle m_FullscreenPso;

	gfx::BufferHandle m_FrameCbvBuf;
	SimpleRenderer_PerFrame m_FrameCB;
};

//

Dev::Dev(int argc, char** argv) 
	: WindowedApp(argc, argv)
	, m_GraphicsContext(nullptr)
{
	VAST_SUBSCRIBE_TO_EVENT("dev", DebugActionEvent, VAST_EVENT_HANDLER_EXP(GetWindow().SetSize(uint2(700, 650))));

	auto windowSize = GetWindow().GetSize();

	gfx::GraphicsParams params;
	params.swapChainSize = windowSize;
	params.swapChainFormat = gfx::TexFormat::RGBA8_UNORM;
	params.backBufferFormat = gfx::TexFormat::RGBA8_UNORM;

	m_GraphicsContext = gfx::GraphicsContext::Create(params);

	GraphicsContext& ctx = *m_GraphicsContext;
	m_Renderer = MakePtr<SimpleRenderer>(ctx);

	// Create triangle PSO and vertex buffer (to be bound bindlessly via push constants).
	CreateTriangleResources();
	
	// Create PSO to be used by the cube and sphere.
	PipelineDesc meshPipelineDesc =
	{
		.vs = {.type = ShaderType::VERTEX, .shaderName = "mesh.hlsl", .entryPoint = "VS_Main"},
		.ps = {.type = ShaderType::PIXEL,  .shaderName = "mesh.hlsl", .entryPoint = "PS_Main"},
		.renderPassLayout = 
		{ 
			.rtFormats = { ctx.GetTextureFormat(m_Renderer->GetColorRT())  },
			.dsFormat = { ctx.GetTextureFormat(m_Renderer->GetDepthRT()) },
		},
	};
	m_MeshPso = ctx.CreatePipeline(meshPipelineDesc);
	m_FrameCbvBufProxy = ctx.LookupShaderResource(m_MeshPso, "FrameConstantBuffer");
	m_MeshCbvBufProxy = ctx.LookupShaderResource(m_MeshPso, "ObjectConstantBuffer");

	CreateCubeResources();
	CreateSphereResources();
}

void Dev::CreateTriangleResources()
{
	GraphicsContext& ctx = *m_GraphicsContext;

	PipelineDesc trianglePipelineDesc =
	{
		.vs = {.type = ShaderType::VERTEX, .shaderName = "triangle.hlsl", .entryPoint = "VS_Main"},
		.ps = {.type = ShaderType::PIXEL,  .shaderName = "triangle.hlsl", .entryPoint = "PS_Main"},
		.depthStencilState = DepthStencilState::Preset::kDisabled,
		.renderPassLayout = { .rtFormats = { ctx.GetTextureFormat(m_Renderer->GetColorRT()) } },
	};
	m_TrianglePso = ctx.CreatePipeline(trianglePipelineDesc);

	BufferDesc vtxBufDesc = AllocVertexBufferDesc(sizeof(s_TriangleVertexData), sizeof(s_TriangleVertexData[0]), true);
	m_TriangleVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_TriangleVertexData, sizeof(s_TriangleVertexData));
	m_TriangleVtxBufIdx = ctx.GetBindlessIndex(m_TriangleVtxBuf);
}

void Dev::CreateCubeResources()
{
	GraphicsContext& ctx = *m_GraphicsContext;

	BufferDesc vtxBufDesc = AllocVertexBufferDesc(sizeof(s_CubeVertexData), sizeof(s_CubeVertexData[0]));
	m_CubeVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_CubeVertexData, sizeof(s_CubeVertexData));

	m_CubeColorTex = ctx.CreateTexture("image.tga");
	m_CubeCB =
	{
		.model = float4x4(),
		.vtxBufIdx = ctx.GetBindlessIndex(m_CubeVtxBuf),
		.colorTexIdx = ctx.GetBindlessIndex(m_CubeColorTex),
		.colorSamplerIdx = IDX(SamplerState::POINT_CLAMP),
	};

	BufferDesc cbvBufDesc = AllocCbvBufferDesc(sizeof(MeshCB));
	m_CubeCbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_CubeCB, sizeof(MeshCB));
}

void Dev::CreateSphereResources()
{
	GraphicsContext& ctx = *m_GraphicsContext;

	ConstructUVSphere(1.2f, 18, 36, s_SphereVertexData, s_SphereIndexData);

	const uint32 vtxSize = static_cast<uint32>(s_SphereVertexData.size() * sizeof(Vtx3fPos3fNormal2fUv));
	BufferDesc vtxBufDesc = AllocVertexBufferDesc(vtxSize, sizeof(Vtx3fPos3fNormal2fUv));
	m_SphereVtxBuf = ctx.CreateBuffer(vtxBufDesc, s_SphereVertexData.data(), s_SphereVertexData.size() * sizeof(Vtx3fPos3fNormal2fUv));

	BufferDesc idxBufDesc = AllocIndexBufferDesc(static_cast<uint32>(s_SphereIndexData.size()));
	m_SphereIdxBuf = ctx.CreateBuffer(idxBufDesc, s_SphereIndexData.data(), s_SphereIndexData.size() * sizeof(uint16));

	m_SphereColorTex = ctx.CreateTexture("2k_earth_daymap.jpg");
	m_SphereCB =
	{
		.model =float4x4(),
		.vtxBufIdx = ctx.GetBindlessIndex(m_SphereVtxBuf),
		.colorTexIdx = ctx.GetBindlessIndex(m_SphereColorTex),
		.colorSamplerIdx = IDX(SamplerState::LINEAR_CLAMP),
	};

	BufferDesc cbvBufDesc = AllocCbvBufferDesc(sizeof(MeshCB));
	m_SphereCbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_SphereCB, sizeof(MeshCB));
}

Dev::~Dev()
{
	GraphicsContext& ctx = *m_GraphicsContext;

	ctx.DestroyPipeline(m_TrianglePso);
	ctx.DestroyPipeline(m_MeshPso);
	ctx.DestroyTexture(m_CubeColorTex);
	ctx.DestroyTexture(m_SphereColorTex);
	ctx.DestroyBuffer(m_TriangleVtxBuf);
	ctx.DestroyBuffer(m_CubeVtxBuf);
	ctx.DestroyBuffer(m_CubeCbvBuf);
	ctx.DestroyBuffer(m_SphereVtxBuf);
	ctx.DestroyBuffer(m_SphereIdxBuf);
	ctx.DestroyBuffer(m_SphereCbvBuf);

	m_Renderer = nullptr;
	m_GraphicsContext = nullptr;
}

void Dev::Update()
{
	static float rotation = 0.0f;
	rotation += 0.001f;

	float4x4 rotationMatrix = float4x4::rotation_y(rotation);
	m_CubeCB.model = mul(rotationMatrix, float4x4::translation(2.0f, -0.5f, 0.0f));
	m_SphereCB.model = mul(rotationMatrix, float4x4::translation(-2.0f, 0.0f, 0.0f));
}

void Dev::Draw()
{
	GraphicsContext& ctx = *m_GraphicsContext;

	m_Renderer->BeginFrame();
	OnGUI();

	if (s_ReloadTriangle)
	{
		s_ReloadTriangle = false;
		ctx.UpdatePipeline(m_TrianglePso);
	}

	if (s_ReloadMesh)
	{
		s_ReloadMesh = false;
		ctx.UpdatePipeline(m_MeshPso);
	}

	if (s_UpdateTriangle)
	{
		s_UpdateTriangle = false;
		ctx.UpdateBuffer(m_TriangleVtxBuf, &s_TriangleVertexData, sizeof(s_TriangleVertexData));
	}

 	ctx.UpdateBuffer(m_CubeCbvBuf, &m_CubeCB, sizeof(MeshCB));
 	ctx.UpdateBuffer(m_SphereCbvBuf, &m_SphereCB, sizeof(MeshCB));

	RenderPassTargets trianglePassTargets;
	trianglePassTargets.rt[0] = {.h = m_Renderer->GetColorRT(), .loadOp = LoadOp::CLEAR };

	ctx.BeginRenderPass(m_TrianglePso, trianglePassTargets);
	{
		ctx.SetPushConstants(&m_TriangleVtxBufIdx, sizeof(uint32));
		ctx.Draw(3);
	}
	ctx.EndRenderPass();

	RenderPassTargets meshPassTargets;
	meshPassTargets.rt[0] = {.h = m_Renderer->GetColorRT(), .nextUsage = ResourceState::PIXEL_SHADER_RESOURCE };
	meshPassTargets.ds = {.h = m_Renderer->GetDepthRT(), .loadOp = LoadOp::CLEAR, .storeOp = StoreOp::DISCARD };

	ctx.BeginRenderPass(m_MeshPso, meshPassTargets);
	{
		ctx.SetConstantBufferView(m_Renderer->GetFrameCBV(), m_FrameCbvBufProxy);

		if (ctx.GetIsReady(m_CubeVtxBuf) && ctx.GetIsReady(m_CubeColorTex))
		{
			ctx.SetConstantBufferView(m_CubeCbvBuf, m_MeshCbvBufProxy);
			ctx.Draw(36);
		}

		if (ctx.GetIsReady(m_SphereVtxBuf) && ctx.GetIsReady(m_SphereIdxBuf) && ctx.GetIsReady(m_SphereColorTex))
		{
			ctx.SetConstantBufferView(m_SphereCbvBuf, m_MeshCbvBufProxy);
			ctx.SetIndexBuffer(m_SphereIdxBuf, 0, IndexBufFormat::R16_UINT);
			ctx.DrawIndexed(static_cast<uint32>(s_SphereIndexData.size()));
		}
	}
	ctx.EndRenderPass();

	m_Renderer->EndFrame();
}

void Dev::OnGUI()
{
	ImGui::ShowDemoWindow();

	if (ImGui::Begin("vast UI", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::PushItemWidth(300);
		ImGui::Text("Triangle Vertex Positions");
		if (ImGui::SliderFloat2("##1", (float*)&s_TriangleVertexData[0].pos, -1.0f, 1.0f)) s_UpdateTriangle = true;
		if (ImGui::SliderFloat2("##2", (float*)&s_TriangleVertexData[1].pos, -1.0f, 1.0f)) s_UpdateTriangle = true;
		if (ImGui::SliderFloat2("##3", (float*)&s_TriangleVertexData[2].pos, -1.0f, 1.0f)) s_UpdateTriangle = true;
		ImGui::Text("Triangle Vertex Colors");
		if (ImGui::ColorEdit3("##1", (float*)&s_TriangleVertexData[0].col)) s_UpdateTriangle = true;
		if (ImGui::ColorEdit3("##2", (float*)&s_TriangleVertexData[1].col)) s_UpdateTriangle = true;
		if (ImGui::ColorEdit3("##3", (float*)&s_TriangleVertexData[2].col)) s_UpdateTriangle = true;
		ImGui::PopItemWidth();

		ImGui::Separator();
		if (ImGui::Button("Reload Triangle Shader"))
		{
			s_ReloadTriangle = true;
		}
		if (ImGui::Button("Reload Mesh Shader"))
		{
			s_ReloadMesh = true;
		}
	}
	ImGui::End();
}
