#include "vastpch.h"
#include "Graphics/Resources.h"

namespace vast::gfx
{

	BufferDesc AllocVertexBufferDesc(uint32 size, uint32 stride, bool bCpuAccess /* = false */, bool bBindless /* = true */)
	{
		return BufferDesc
		{
			.size = size,
			.stride = stride,
			.viewFlags = bBindless ? BufViewFlags::SRV : BufViewFlags::NONE,
			.cpuAccess = bCpuAccess ? BufCpuAccess::WRITE : BufCpuAccess::NONE,
			.usage = bCpuAccess ? ResourceUsage::DYNAMIC : ResourceUsage::STATIC,
			.isRawAccess = bBindless,
		};
	}

	BufferDesc AllocIndexBufferDesc(uint32 numIndices, IndexBufFormat format /* = IndexBufFormat::R16_UINT */)
	{
		VAST_ASSERT(format != IndexBufFormat::UNKNOWN);
		const uint32 indexSize = (format == IndexBufFormat::R32_UINT) ? sizeof(uint32) : sizeof(uint16);

		return BufferDesc
		{
			.size = numIndices * indexSize,
			.stride = indexSize,
			.viewFlags = BufViewFlags::NONE,
			.cpuAccess = BufCpuAccess::NONE,
			.usage = ResourceUsage::STATIC,
			.isRawAccess = false,
		};
	}

	BufferDesc AllocCbvBufferDesc(uint32 size, bool bCpuAccess /* = true */)
	{
		return BufferDesc
		{
			.size = size,
			.stride = 0,
			.viewFlags = BufViewFlags::CBV,
			.cpuAccess = bCpuAccess ? BufCpuAccess::WRITE : BufCpuAccess::NONE,
			.usage = bCpuAccess ? ResourceUsage::DYNAMIC : ResourceUsage::STATIC,
			.isRawAccess = false,
		};
	}

#ifdef VAST_DEBUG
	static void ValidateDepthStencilTargetFormat(TexFormat format, bool invert = false)
	{
		bool bIsDepthStencilFormat = (format == TexFormat::D32_FLOAT);
		if (invert)
		{
			VAST_ASSERTF(!bIsDepthStencilFormat, "Attempted to create a RTV with a DSV format");
		}
		else
		{
			VAST_ASSERTF(bIsDepthStencilFormat, "Attempted to create a DSV with a RTV format");
		}
	}

	static void ValidateRenderTargetFormat(TexFormat format)
	{
		ValidateDepthStencilTargetFormat(format, true);
	}
#endif

	TextureDesc AllocRenderTargetDesc(TexFormat format, uint2 dimensions, float4 clear /* = float4(0, 0, 0, 1) */)
	{
#ifdef VAST_DEBUG
		ValidateRenderTargetFormat(format);
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

	TextureDesc AllocDepthStencilTargetDesc(TexFormat format, uint2 dimensions, ClearDepthStencil clear /* = { 1.0f, 0 } */)
	{
#ifdef VAST_DEBUG
		ValidateDepthStencilTargetFormat(format);
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
