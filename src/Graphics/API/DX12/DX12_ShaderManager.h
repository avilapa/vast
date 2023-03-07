#pragma once

#include "Graphics/ShaderResourceProxy.h"
#include "Graphics/API/DX12/DX12_Common.h"

#include "dx12/DirectXAgilitySDK/include/d3d12shader.h"

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace vast::gfx
{

	class DX12ShaderManager
	{
	public:
		DX12ShaderManager();
		~DX12ShaderManager();

		Ref<DX12Shader> LoadShader(const ShaderDesc& desc);

		ID3DBlob* CreateRootSignatureFromReflection(DX12Pipeline* pipeline) const;
		Vector<D3D12_SIGNATURE_PARAMETER_DESC> GetInputParametersFromReflection(ID3D12ShaderReflection* reflection) const;

	private:
		Ref<DX12Shader> CompileShader(const ShaderDesc& desc, const std::string& key);

		IDxcUtils* m_DxcUtils;
		IDxcCompiler3* m_DxcCompiler;
		IDxcIncludeHandler* m_DxcIncludeHandler;

		std::string MakeShaderKey(const std::string& shaderName, const std::string& entryPoint) const;
		bool IsShaderRegistered(const std::string key) const;

		std::unordered_map<std::string, uint32> m_ShaderKeys;
		Vector<Ref<DX12Shader>> m_Shaders;
	};

}