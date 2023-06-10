#include "SampleBase.h"

using namespace vast;
using namespace vast::gfx;

SampleBase::SampleBase(GraphicsContext& ctx)
	: m_GraphicsContext(ctx)
{
}

SampleBase::~SampleBase()
{
}

void SampleBase::Update()
{
}

void SampleBase::Draw()
{
}

void SampleBase::OnGUI()
{
}

//

SampleBase3D::SampleBase3D(GraphicsContext& ctx)
	: SampleBase(ctx)
{
	auto windowSize = m_GraphicsContext.GetSwapChainSize();

	// Create intermediate color and depth buffers
	TextureDesc colorTargetDesc =
	{
		.format		= TexFormat::RGBA8_UNORM,
		.width		= windowSize.x,
		.height		= windowSize.y,
		.viewFlags	= TexViewFlags::RTV | TexViewFlags::SRV,
		.clear		= ClearValue(float4(0.6f, 0.2f, 0.9f, 1.0f)),
	};

	TextureDesc depthTargetDesc =
	{
		.format		= TexFormat::D32_FLOAT,
		.width		= windowSize.x,
		.height		= windowSize.y,
		.viewFlags	= TexViewFlags::DSV,
		.clear		= ClearValue(1.0f, 1), // TODO: Default stencil value?
	};

	m_ColorRT = ctx.CreateTexture(colorTargetDesc);
	m_DepthRT = ctx.CreateTexture(depthTargetDesc);

	RenderPassLayout fullscreenPass =
	{
		{ m_GraphicsContext.GetBackBufferFormat(), LoadOp::LOAD, StoreOp::STORE, ResourceState::PRESENT }
	};

	m_FullscreenPso = ctx.CreatePipeline(PipelineDesc{
		.vs = {.type = ShaderType::VERTEX, .shaderName = "fullscreen.hlsl", .entryPoint = "VS_Main"},
		.ps = {.type = ShaderType::PIXEL,  .shaderName = "fullscreen.hlsl", .entryPoint = "PS_Main"},
		.renderPassLayout = fullscreenPass,
		.depthStencilState = DepthStencilState::Preset::kDisabled,
	});

	m_ColorTexIdx = m_GraphicsContext.GetBindlessIndex(m_ColorRT);
}

SampleBase3D::~SampleBase3D()
{
	m_GraphicsContext.DestroyTexture(m_ColorRT);
	m_GraphicsContext.DestroyTexture(m_DepthRT);
	m_GraphicsContext.DestroyPipeline(m_FullscreenPso);
}

void SampleBase3D::SetRenderTargets()
{
	m_GraphicsContext.SetRenderTarget(m_ColorRT);
	m_GraphicsContext.SetDepthStencilTarget(m_DepthRT);
}

void SampleBase3D::DrawToBackBuffer()
{
	m_GraphicsContext.BeginRenderPass(m_FullscreenPso);
	{
		m_GraphicsContext.SetPushConstants(&m_ColorTexIdx, sizeof(uint32));
		m_GraphicsContext.DrawFullscreenTriangle();
	}
	m_GraphicsContext.EndRenderPass();
}