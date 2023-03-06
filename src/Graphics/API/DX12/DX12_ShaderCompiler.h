#pragma once

#include "Graphics/ShaderResourceProxy.h"
#include "Graphics/API/DX12/DX12_Common.h"

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace vast::gfx
{

	class DX12ShaderCompiler
	{
	public:
		DX12ShaderCompiler();
		~DX12ShaderCompiler();

		void Compile(const ShaderDesc& desc, DX12Shader* outShader);
		ID3DBlob* CreateRootSignatureFromReflection(DX12Pipeline* pipeline);

	private:
		const wchar_t* GetShaderTarget(ShaderType type);

		IDxcUtils* m_DxcUtils;
		IDxcCompiler3* m_DxcCompiler;
		IDxcIncludeHandler* m_DxcIncludeHandler;
		// TODO: Table with registered shaders?
	};

}