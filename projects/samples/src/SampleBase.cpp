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

void SampleBase::OnWindowResizeEvent(const WindowResizeEvent&)
{
}
