#pragma once

#include "Graphics/API/DX12/DX12_Common.h"

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcBlob;
struct IDxcBlobEncoding;
struct IDxcResult;
struct ID3D12ShaderReflection;

namespace vast::gfx
{

	class DX12ShaderCompiler
	{
	public:
		DX12ShaderCompiler();
		~DX12ShaderCompiler();

		static const wchar_t* ToShaderTarget(ShaderType type);

		IDxcBlobEncoding* LoadShader(std::wstring& fullPath);
		IDxcResult* CompileShader(IDxcBlobEncoding* sourceBlobEncoding, Vector<LPCWSTR>& args);
		ID3D12ShaderReflection* ExtractShaderReflection(IDxcResult* compiledShader);
		IDxcBlob* ExtractShaderOutput(IDxcResult* compiledShader);
		IDxcBlob* ExtractShaderPDB(IDxcResult* compiledShader);

	private:
		IDxcUtils* m_DxcUtils;
		IDxcCompiler3* m_DxcCompiler;
		IDxcIncludeHandler* m_DxcIncludeHandler;
	};

}