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
//
//	> Object Loading from file (.obj).
//	> Scene Loading (.gltf, .usd?).
//	> Delta Time.
//	> Camera Object / Movement.
//
// --------------------------------------------------------------------------------------------- //

VAST_DEFINE_APP_MAIN(Dev)

#define PI 3.14159265358979323846264338327950288

using namespace vast;

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

Dev::Dev(int argc, char** argv) : WindowedApp(argc, argv)
{
	gfx::GraphicsContext& ctx = GetGraphicsContext();

	// Create intermediate color and depth buffers
	{
		auto windowSize = GetWindow().GetSize();

		m_ColorRT = ctx.CreateTexture(gfx::TextureDesc::Builder()
			.SetType(gfx::TextureType::TEXTURE_2D)
			.SetFormat(gfx::Format::RGBA8_UNORM)
			.SetWidth(windowSize.x)
			.SetHeight(windowSize.y)
			.SetViewFlags(gfx::TextureViewFlags::RTV | gfx::TextureViewFlags::SRV)
			.SetRenderTargetClearColor(float4(0.6f, 0.2f, 0.9f, 1.0f)));

		m_DepthRT = ctx.CreateTexture(gfx::TextureDesc::Builder()
			.SetType(gfx::TextureType::TEXTURE_2D)
			.SetFormat(gfx::Format::D32_FLOAT)
			.SetWidth(windowSize.x)
			.SetHeight(windowSize.y)
			.SetViewFlags(gfx::TextureViewFlags::DSV)
			.SetDepthClearValue(1.0f));
	}

	gfx::Format colorTargetFormat = ctx.GetTextureFormat(m_ColorRT);
	gfx::Format depthTargetFormat = gfx::Format::D32_FLOAT; // ctx.GetTextureFormat(m_DepthRT); // TODO: Currently returns typeless
	gfx::Format backBufferFormat = ctx.GetBackBufferFormat();

	// Render triangle to intermediate color buffer. Clear color buffer, since it's the first usage of it in the frame.
	gfx::RenderPassLayout triangleRenderPass = 
	{ 
		{ colorTargetFormat, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, gfx::ResourceState::NONE }
	};
	// Render cube to intermediate color + depth buffers. Clear depth buffer, since  it's the first usage of it in the frame.
	gfx::RenderPassLayout colorDepthPass = 
	{ 
		{ colorTargetFormat, gfx::LoadOp::LOAD, gfx::StoreOp::STORE, gfx::ResourceState::SHADER_RESOURCE },
		{ depthTargetFormat, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, gfx::ResourceState::NONE }
	};
	// Render color buffer to back buffer. Doesn't need to be cleared since we're rendering full screen.
	gfx::RenderPassLayout fullscreenPass = 
	{
		{ backBufferFormat, gfx::LoadOp::LOAD, gfx::StoreOp::STORE, gfx::ResourceState::PRESENT }
	};

	// Create triangle PSO and vertex buffer (to be bound bindlessly via push constants).
	CreateTriangleResources(triangleRenderPass);

	// Create PSO to be used by the cube and sphere.
	m_MeshPso = ctx.CreatePipeline(gfx::PipelineDesc::Builder()
		.SetVertexShader("mesh.hlsl", "VS_Main")
		.SetPixelShader("mesh.hlsl", "PS_Main")
		.SetRasterizerState(gfx::RasterizerState{gfx::FillMode::WIREFRAME, gfx::CullMode::NONE})
		.SetRenderPassLayout(colorDepthPass));
	m_MeshCbvBufProxy = ctx.LookupShaderResource(m_MeshPso, "CB");

	CreateCubeResources();
	CreateSphereResources();

	m_FullscreenPso = ctx.CreatePipeline(gfx::PipelineDesc::Builder()
		.SetVertexShader("fullscreen.hlsl", "VS_Main")
		.SetPixelShader("fullscreen.hlsl", "PS_Main")
		.SetDepthStencilState(gfx::DepthStencilState::Preset::kDisabled)
		.SetRenderPassLayout(fullscreenPass));
	m_ColorTexIdx = ctx.GetBindlessIndex(m_ColorRT);
}

void Dev::CreateTriangleResources(const gfx::RenderPassLayout& pass)
{
	gfx::GraphicsContext& ctx = GetGraphicsContext();

	m_TrianglePso = ctx.CreatePipeline(gfx::PipelineDesc::Builder()
		.SetVertexShader("triangle.hlsl", "VS_Main")
		.SetPixelShader("triangle.hlsl", "PS_Main")
		.SetDepthStencilState(gfx::DepthStencilState::Preset::kDisabled)
		.SetRenderPassLayout(pass));

	auto vtxBufDesc = gfx::BufferDesc::Builder()
		.SetSize(sizeof(s_TriangleVertexData)).SetStride(sizeof(s_TriangleVertexData[0]))
		.SetViewFlags(gfx::BufferViewFlags::SRV)
		.SetCpuAccess(gfx::BufferCpuAccess::WRITE)
		.SetUsage(gfx::ResourceUsage::DYNAMIC)
		.SetIsRawAccess(true);
	m_TriangleVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_TriangleVertexData, sizeof(s_TriangleVertexData));
	m_TriangleVtxBufIdx = ctx.GetBindlessIndex(m_TriangleVtxBuf);
}

void Dev::CreateCubeResources()
{
	gfx::GraphicsContext& ctx = GetGraphicsContext();

	auto vtxBufDesc = gfx::BufferDesc::Builder()
		.SetSize(sizeof(s_CubeVertexData)).SetStride(sizeof(s_CubeVertexData[0]))
		.SetViewFlags(gfx::BufferViewFlags::SRV)
		.SetCpuAccess(gfx::BufferCpuAccess::NONE)
		.SetIsRawAccess(true);
	m_CubeVtxBuf = ctx.CreateBuffer(vtxBufDesc, &s_CubeVertexData, sizeof(s_CubeVertexData));

	m_CubeColorTex = ctx.CreateTexture("image.tga");

	auto windowSize = GetWindow().GetSize();
	float fieldOfView = float(PI) / 4.0f;
	float aspectRatio = (float)windowSize.x / (float)windowSize.y;
	m_CubeCB =
	{
		float4x4(),
		float4x4::look_at({ -3.0f, 3.0f, -8.0f }, float3(0), float3(0, 1, 0)),
		float4x4::perspective(hlslpp::projection(hlslpp::frustum::field_of_view_x(fieldOfView, aspectRatio, 0.001f, 1000.0f), hlslpp::zclip::t::zero)),
		{ -3.0f, 3.0f, -8.0f },
		ctx.GetBindlessIndex(m_CubeVtxBuf),
		ctx.GetBindlessIndex(m_CubeColorTex),
		IDX(gfx::SamplerState::POINT_CLAMP),
	};

	auto cbvBufDesc = gfx::BufferDesc::Builder()
		.SetSize(sizeof(MeshCB))
		.SetViewFlags(gfx::BufferViewFlags::CBV)
		.SetCpuAccess(gfx::BufferCpuAccess::WRITE)
		.SetUsage(gfx::ResourceUsage::DYNAMIC)
		.SetIsRawAccess(true);
	m_CubeCbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_CubeCB, sizeof(MeshCB));
}

void Dev::CreateSphereResources()
{
	gfx::GraphicsContext& ctx = GetGraphicsContext();

	ConstructUVSphere(1.2f, 18, 36, s_SphereVertexData, s_SphereIndexData);

	auto vtxBufDesc = gfx::BufferDesc::Builder()
		.SetSize(static_cast<uint32>(s_SphereVertexData.size()) * sizeof(Vtx3fPos3fNormal2fUv)).SetStride(sizeof(Vtx3fPos3fNormal2fUv))
		.SetViewFlags(gfx::BufferViewFlags::SRV)
		.SetCpuAccess(gfx::BufferCpuAccess::NONE)
		.SetIsRawAccess(true);
	m_SphereVtxBuf = ctx.CreateBuffer(vtxBufDesc, s_SphereVertexData.data(), s_SphereVertexData.size() * sizeof(Vtx3fPos3fNormal2fUv));

	auto idxBufDesc = gfx::BufferDesc::Builder()
		.SetSize(static_cast<uint32>(s_SphereIndexData.size()) * sizeof(uint16)).SetStride(sizeof(uint16))
		.SetCpuAccess(gfx::BufferCpuAccess::NONE);
	m_SphereIdxBuf = ctx.CreateBuffer(idxBufDesc, s_SphereIndexData.data(), s_SphereIndexData.size() * sizeof(uint16));

	m_SphereColorTex = ctx.CreateTexture("2k_earth_daymap.jpg");

	auto windowSize = GetWindow().GetSize();
	float fieldOfView = float(PI) / 4.0f;
	float aspectRatio = (float)windowSize.x / (float)windowSize.y;
	m_SphereCB =
	{
		float4x4(),
		float4x4::look_at({ -3.0f, 3.0f, -8.0f }, float3(0), float3(0, 1, 0)),
		float4x4::perspective(hlslpp::projection(hlslpp::frustum::field_of_view_x(fieldOfView, aspectRatio, 0.001f, 1000.0f), hlslpp::zclip::t::zero)),
		{ -3.0f, 3.0f, -8.0f },
		ctx.GetBindlessIndex(m_SphereVtxBuf),
		ctx.GetBindlessIndex(m_SphereColorTex),
		IDX(gfx::SamplerState::LINEAR_CLAMP),
	};

	auto cbvBufDesc = gfx::BufferDesc::Builder()
		.SetSize(sizeof(MeshCB))
		.SetViewFlags(gfx::BufferViewFlags::CBV)
		.SetCpuAccess(gfx::BufferCpuAccess::WRITE)
		.SetUsage(gfx::ResourceUsage::DYNAMIC)
		.SetIsRawAccess(true);
	m_SphereCbvBuf = ctx.CreateBuffer(cbvBufDesc, &m_SphereCB, sizeof(MeshCB));
}

Dev::~Dev()
{
	gfx::GraphicsContext& ctx = GetGraphicsContext();

	ctx.DestroyPipeline(m_FullscreenPso);
	ctx.DestroyPipeline(m_TrianglePso);
	ctx.DestroyPipeline(m_MeshPso);
	ctx.DestroyTexture(m_ColorRT);
	ctx.DestroyTexture(m_DepthRT);
	ctx.DestroyTexture(m_CubeColorTex);
	ctx.DestroyTexture(m_SphereColorTex);
	ctx.DestroyBuffer(m_TriangleVtxBuf);
	ctx.DestroyBuffer(m_CubeVtxBuf);
	ctx.DestroyBuffer(m_CubeCbvBuf);
	ctx.DestroyBuffer(m_SphereVtxBuf);
	ctx.DestroyBuffer(m_SphereIdxBuf);
	ctx.DestroyBuffer(m_SphereCbvBuf);
}

void Dev::Update()
{
	static float rotation = 0.0f;
	rotation += 0.001f;

	float4x4 rotationMatrix = float4x4::rotation_y(rotation);
	m_CubeCB.model = mul(rotationMatrix, float4x4::translation(2.0f, -0.5f, 0.0f));
	m_SphereCB.model = mul(rotationMatrix, float4x4::translation(-2.0f, 0.0f, 0.0f));

	OnGUI();
}

void Dev::Render()
{
	gfx::GraphicsContext& ctx = GetGraphicsContext();

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

	// Update Constant Buffers
	ctx.UpdateBuffer(m_CubeCbvBuf, &m_CubeCB, sizeof(MeshCB));
	ctx.UpdateBuffer(m_SphereCbvBuf, &m_SphereCB, sizeof(MeshCB));

	// Draw triangle to intermediate buffer
	ctx.SetRenderTarget(m_ColorRT);
	ctx.BeginRenderPass(m_TrianglePso);
	{
		ctx.SetPushConstants(&m_TriangleVtxBufIdx, sizeof(uint32));
		ctx.Draw(3);
	}
	ctx.EndRenderPass();

	// Draw cube and sphere to intermediate buffer + depth
	ctx.SetRenderTarget(m_ColorRT);
	ctx.SetDepthStencilTarget(m_DepthRT);
	ctx.BeginRenderPass(m_MeshPso);
	{
		if (ctx.GetIsReady(m_CubeVtxBuf) && ctx.GetIsReady(m_CubeColorTex))
		{
			ctx.SetShaderResource(m_CubeCbvBuf, m_MeshCbvBufProxy);
			ctx.Draw(36);
		}

		if (ctx.GetIsReady(m_SphereVtxBuf) && ctx.GetIsReady(m_SphereIdxBuf) && ctx.GetIsReady(m_SphereColorTex))
		{
			ctx.SetShaderResource(m_SphereCbvBuf, m_MeshCbvBufProxy);
			ctx.SetIndexBuffer(m_SphereIdxBuf, 0, gfx::Format::R16_UINT);
			ctx.DrawIndexed(static_cast<uint32>(s_SphereIndexData.size()));
		}
	}
	ctx.EndRenderPass();

	ctx.BeginRenderPass(m_FullscreenPso);
	{
		ctx.SetPushConstants(&m_ColorTexIdx, sizeof(uint32));
		ctx.DrawFullscreenTriangle();
	}
	ctx.EndRenderPass();
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
