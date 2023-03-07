#pragma once

#include "Graphics/GraphicsContext.h"

namespace vast::gfx
{
	static const uint32 IMGUI_NUM_FRAMES_IN_FLIGHT = 2; // TODO

	class ImguiRenderer
	{
	public:
		// TODO: HWND is platform specific!
		ImguiRenderer(gfx::GraphicsContext& context, HWND windowHandle = ::GetActiveWindow());
		~ImguiRenderer();

		void BeginFrame(); // TODO: Is this to be called at the beginning of the frame or the render pass?
		void ExecuteRenderPass();

	private:
		gfx::GraphicsContext& ctx;

		Array<uint32, IMGUI_NUM_FRAMES_IN_FLIGHT> m_VtxSize;
		Array<uint32, IMGUI_NUM_FRAMES_IN_FLIGHT> m_IdxSize;
		Array<gfx::BufferHandle, IMGUI_NUM_FRAMES_IN_FLIGHT> m_VtxBuf;
		Array<gfx::BufferHandle, IMGUI_NUM_FRAMES_IN_FLIGHT> m_IdxBuf;
		gfx::TextureHandle m_FontTex;
		gfx::PipelineHandle m_Pipeline;
	};

}