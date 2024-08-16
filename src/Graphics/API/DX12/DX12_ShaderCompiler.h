#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcBlob;
struct IDxcBlobEncoding;
struct IDxcResult;
struct ID3D12ShaderReflection;

namespace vast
{
	struct ShaderCompilerArguments
	{
		ShaderType shaderType;
		std::wstring shaderName;
		std::wstring shaderEntryPoint;
		Vector<std::wstring> includeDirectories;
		Vector<std::wstring> additionalDefines;
	};

	class DX12ShaderCompiler
	{
	public:
		DX12ShaderCompiler();
		~DX12ShaderCompiler();

		IDxcBlobEncoding* LoadShader(const std::wstring& fullPath);
		IDxcResult* CompileShader(IDxcBlobEncoding* sourceBlobEncoding, const ShaderCompilerArguments& sca);
		ID3D12ShaderReflection* ExtractShaderReflection(IDxcResult* compiledShader);
		IDxcBlob* ExtractShaderOutput(IDxcResult* compiledShader);
		IDxcBlob* ExtractShaderPDB(IDxcResult* compiledShader);

	private:
		IDxcUtils* m_DxcUtils;
		IDxcCompiler3* m_DxcCompiler;
		IDxcIncludeHandler* m_DxcIncludeHandler;
	};

}