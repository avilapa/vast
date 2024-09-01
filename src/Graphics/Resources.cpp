#include "vastpch.h"
#include "Graphics/Resources.h"

namespace vast
{

	BufferDesc AllocVertexBufferDesc(uint32 size, uint32 stride, bool bBindless /* = true */, ResourceUsage usage /* = ResourceUsage::DEFAULT */)
	{
		return BufferDesc
		{
			.size = size,
			.stride = stride,
			.viewFlags = bBindless ? BufViewFlags::SRV : BufViewFlags::NONE,
			.usage = usage,
			.bBindless = bBindless,
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
			.bBindless = false,
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
			.bBindless = false,
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
			.bBindless = false,
		};
	}

	TextureDesc AllocRenderTargetDesc(TexFormat format, uint2 dimensions, float4 clear /* = DEFAULT_CLEAR_COLOR_VALUE */)
	{
		VAST_VERIFYF(!IsTexFormatDepth(format), "Attempted to create a RTV with a DSV format");

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
		VAST_VERIFYF(IsTexFormatDepth(format), "Attempted to create a DSV with a RTV format");

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

	static ShaderDesc AllocShaderDesc(const ShaderType shaderType, const char* shaderName, const char* filePath, const char* entryPoint)
	{
		// TODO: Assert if shaderName doesn't include .hlsl extension

		return ShaderDesc
		{
			.type = shaderType,
			.filePath = filePath,
			.shaderName = shaderName,
			.entryPoint = entryPoint,
		};
	}

	ShaderDesc AllocComputeShaderDesc(const char* shaderName, const char* filePath /* = VAST_DEFAULT_SHADERS_SOURCE_PATH */, const char* entryPoint /* = "CS_Main" */)
	{
		return AllocShaderDesc(ShaderType::COMPUTE, shaderName, filePath, entryPoint);
	}

	ShaderDesc AllocVertexShaderDesc(const char* shaderName, const char* filePath /* = VAST_DEFAULT_SHADERS_SOURCE_PATH */, const char* entryPoint /* = "VS_Main" */)
	{
		return AllocShaderDesc(ShaderType::VERTEX, shaderName, filePath, entryPoint);
	}

	ShaderDesc AllocPixelShaderDesc(const char* shaderName, const char* filePath /* = VAST_DEFAULT_SHADERS_SOURCE_PATH */, const char* entryPoint /* = "PS_Main" */)
	{
		return AllocShaderDesc(ShaderType::PIXEL, shaderName, filePath, entryPoint);
	}

}
