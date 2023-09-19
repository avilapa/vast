#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{

	class ImguiRenderer
	{
	public:
		// TODO: HWND is platform specific!
		ImguiRenderer(gfx::GraphicsContext& ctx, HWND windowHandle = ::GetActiveWindow());
		~ImguiRenderer();

		void BeginCommandRecording();
		void EndCommandRecording();
		void Render();

	private:
		gfx::GraphicsContext& ctx;
		gfx::PipelineHandle m_Pipeline;
		gfx::TextureHandle m_FontTex;
		gfx::ShaderResourceProxy m_TextureProxy;
	};

}