#include "SampleBase.h"

using namespace vast;

SampleBase::SampleBase(gfx::GraphicsContext& ctx)
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

SampleBase3D::SampleBase3D(gfx::GraphicsContext& ctx)
	: SampleBase(ctx)
{
	auto windowSize = m_GraphicsContext.GetSwapChainSize();

	// Create intermediate color and depth buffers
	m_ColorRT = m_GraphicsContext.CreateTexture(gfx::TextureDesc::Builder()
		.Type(gfx::TextureType::TEXTURE_2D)
		.Format(gfx::Format::RGBA8_UNORM)
		.Width(windowSize.x)
		.Height(windowSize.y)
		.ViewFlags(gfx::TextureViewFlags::RTV | gfx::TextureViewFlags::SRV)
		.ClearColor(float4(0.6f, 0.2f, 0.9f, 1.0f)));

	m_DepthRT = m_GraphicsContext.CreateTexture(gfx::TextureDesc::Builder()
		.Type(gfx::TextureType::TEXTURE_2D)
		.Format(gfx::Format::D32_FLOAT)
		.Width(windowSize.x)
		.Height(windowSize.y)
		.ViewFlags(gfx::TextureViewFlags::DSV)
		.ClearDepth(1.0f));

	gfx::RenderPassLayout fullscreenPass =
	{
		{ m_GraphicsContext.GetBackBufferFormat(), gfx::LoadOp::LOAD, gfx::StoreOp::STORE, gfx::ResourceState::PRESENT }
	};

	m_FullscreenPso = m_GraphicsContext.CreatePipeline(gfx::PipelineDesc::Builder()
		.VS("fullscreen.hlsl", "VS_Main")
		.PS("fullscreen.hlsl", "PS_Main")
		.DepthStencil(gfx::DepthStencilState::Preset::kDisabled)
		.RenderPass(fullscreenPass));

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