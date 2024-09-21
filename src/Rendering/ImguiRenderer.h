#pragma once

#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast
{

	class GraphicsContext;

	class ImguiRenderer
	{
	public:
		ImguiRenderer(GraphicsContext& ctx, Window& window);
		~ImguiRenderer();

		void BeginCommandRecording();
		void EndCommandRecording();
		void Render();

	private:
		void OnWindowResizeEvent(const WindowResizeEvent& event);

	private:
		GraphicsContext& ctx;

		PipelineHandle m_Pipeline;
		ShaderResourceProxy m_CbvProxy;
		TextureHandle m_FontTex;
	};

}