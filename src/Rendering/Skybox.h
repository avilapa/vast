#pragma once

#include "Graphics/Resources.h"
#include "Rendering/Camera.h"

namespace vast
{

	class GraphicsContext;
	
	class Skybox
	{
	public:
		Skybox(GraphicsContext& ctx, TexFormat rtFormat, TexFormat dsFormat, bool bUseReverseZ = VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z);
		~Skybox();

		void Render(TextureHandle environmentMap, const RenderTargetDesc& rt, const RenderTargetDesc& ds, const PerspectiveCamera& camera);
		void Render(TextureHandle environmentMap, const RenderTargetDesc& rt, const RenderTargetDesc& ds, const float4x4& viewMatrix, const float4x4& projMatrix);

	private:
		GraphicsContext& ctx;

		PipelineHandle m_PSO;
		BufferHandle m_CubeVtxBuf;
		BufferHandle m_CubeIdxBuf;

		bool m_bUsingReverseZ;
	};

}
