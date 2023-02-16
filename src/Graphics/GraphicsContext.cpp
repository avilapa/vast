#include "vastpch.h"
#include "Graphics/GraphicsContext.h"

#ifdef VAST_PLATFORM_WINDOWS
#include "Graphics/API/DX12/DX12_GraphicsContext.h"
#else
#error "Invalid Platform: Unknown Platform"
#endif

namespace vast::gfx
{

	Ptr<GraphicsContext> GraphicsContext::Create(const GraphicsParams& params /* = GraphicsParams() */)
	{
		VAST_INFO("[gfx] Creating graphics context.");
#ifdef VAST_PLATFORM_WINDOWS
		return MakePtr<DX12GraphicsContext>(params);
#else
		VAST_ASSERTF(0, "Invalid Platform: Unknown Platform");
		return nullptr;
#endif
	}

}
