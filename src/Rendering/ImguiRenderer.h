#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{
	class ImguiRenderer
	{
	public:
		// TODO: HWND is platform specific!
		ImguiRenderer(gfx::GraphicsContext& context, HWND windowHandle = ::GetActiveWindow());
		~ImguiRenderer();

		void BeginFrame();
		void EndFrame();

	private:
		gfx::GraphicsContext& ctx;

		gfx::TextureHandle m_FontTex;
		gfx::ShaderResourceProxy m_FontTexProxy;
		gfx::PipelineHandle m_Pipeline;
	};

}