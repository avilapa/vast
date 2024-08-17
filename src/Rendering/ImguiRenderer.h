#pragma once

#include "Graphics/Resources.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast
{

	class GraphicsContext;

	class ImguiRenderer
	{
	public:
		// TODO: HWND is platform specific!
		ImguiRenderer(GraphicsContext& ctx, HWND windowHandle = ::GetActiveWindow());
		~ImguiRenderer();

		void BeginCommandRecording();
		void EndCommandRecording();
		void Render();

	private:
		GraphicsContext& ctx;
		PipelineHandle m_Pipeline;
		ShaderResourceProxy m_CbvProxy;
		TextureHandle m_FontTex;
	};

}