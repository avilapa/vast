#include "vastpch.h"
#include "Graphics/Resources.h"

namespace vast::gfx
{

	BufferDesc AllocVertexBufferDesc(uint32 size, uint32 stride, ResourceUsage usage /* = ResourceUsage::DEFAULT */, bool bBindless /* = true */)
	{
		return BufferDesc
		{
			.size = size,
			.stride = stride,
			.viewFlags = bBindless ? BufViewFlags::SRV : BufViewFlags::NONE,
			.usage = usage,
			.isRawAccess = bBindless,
		};
	}

	BufferDesc AllocIndexBufferDesc(uint32 numIndices, IndexBufFormat format /* = IndexBufFormat::R16_UINT */, ResourceUsage usage /* = ResourceUsage::DEFAULT */)
	{
		VAST_ASSERT(format != IndexBufFormat::UNKNOWN);
		const uint32 indexSize = (format == IndexBufFormat::R32_UINT) ? sizeof(uint32) : sizeof(uint16);

		return BufferDesc
		{
			.size = numIndices * indexSize,
			.stride = indexSize,
			.viewFlags = BufViewFlags::NONE,
			.usage = usage,
			.isRawAccess = false,
		};
	}

	BufferDesc AllocCbvBufferDesc(uint32 size, ResourceUsage usage /* = ResourceUsage::UPLOAD */)
	{
		return BufferDesc
		{
			.size = size,
			.stride = 0,
			.viewFlags = BufViewFlags::CBV,
			.usage = usage,
			.isRawAccess = false,
		};
	}
	
	BufferDesc AllocStructuredBufferDesc(uint32 size, uint32 stride, ResourceUsage usage /* = ResourceUsage::DEFAULT */)
	{
		return BufferDesc
		{
			.size = size,
			.stride = stride,
			.viewFlags = BufViewFlags::SRV,
			.usage = usage,
			.isRawAccess = false,
		};
	}

	TextureDesc AllocRenderTargetDesc(TexFormat format, uint2 dimensions, float4 clear /* = DEFAULT_CLEAR_COLOR_VALUE */)
	{
#ifdef VAST_DEBUG
		VAST_ASSERTF(!IsTexFormatDepth(format), "Attempted to create a RTV with a DSV format");
#endif
		return TextureDesc
		{
			.type = TexType::TEXTURE_2D,
			.format = format,
			.width  = dimensions.x,
			.height = dimensions.y,
			.depthOrArraySize = 1,
			.mipCount = 1,
			.viewFlags = TexViewFlags::RTV | TexViewFlags::SRV,
			.clear = {.color = clear },
		};
	}

	TextureDesc AllocDepthStencilTargetDesc(TexFormat format, uint2 dimensions, ClearDepthStencil clear /* = { DEFAULT_CLEAR_DEPTH_VALUE, 0 } */)
	{
#ifdef VAST_DEBUG
		VAST_ASSERTF(IsTexFormatDepth(format), "Attempted to create a DSV with a RTV format");
#endif
		return TextureDesc
		{
			.type = TexType::TEXTURE_2D,
			.format = format,
			.width  = dimensions.x,
			.height = dimensions.y,
			.depthOrArraySize = 1,
			.mipCount = 1,
			.viewFlags = TexViewFlags::DSV, // TODO: SRV?
			.clear = {.ds = clear },
		};
	}
}
