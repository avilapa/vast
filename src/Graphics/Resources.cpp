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

}
