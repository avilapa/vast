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

		void AddGlobalShaderDefine(const std::string& name, const std::string& value);

		Ref<DX12Shader> LoadShader(const ShaderDesc& desc);
		bool ReloadShader(Ref<DX12Shader> shader);

		ID3DBlob* CreateRootSignatureFromReflection(DX12Pipeline& pipeline) const;
		Vector<D3D12_SIGNATURE_PARAMETER_DESC> GetInputParametersFromReflection(ID3D12ShaderReflection* reflection) const;

	private:
		IDxcUtils* m_DxcUtils;
		IDxcCompiler3* m_DxcCompiler;
		IDxcIncludeHandler* m_DxcIncludeHandler;

		bool CompileShader(const ShaderDesc& desc, DX12Shader* outShader);

		std::string MakeShaderKey(const std::string& shaderName, const std::string& entryPoint) const;
		bool IsShaderRegistered(const std::string key) const;

		std::unordered_map<std::string, uint32> m_ShaderKeys;
		Vector<std::pair<Ref<DX12Shader>, ShaderDesc>> m_Shaders;

		Vector<std::wstring> m_SharedCompilerArgs;
	};

}