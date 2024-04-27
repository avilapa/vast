#include "vastpch.h"
#include "Graphics/API/DX12/DX12_ShaderCompiler.h"

#include "dx12/DirectXShaderCompiler/inc/dxcapi.h"
#include "dx12/DirectXAgilitySDK/include/d3d12shader.h"

namespace vast::gfx
{

	DX12ShaderCompiler::DX12ShaderCompiler()
		: m_DxcUtils(nullptr)
		, m_DxcCompiler(nullptr)
		, m_DxcIncludeHandler(nullptr)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Shader Compiler");
		DX12Check(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_DxcUtils)));
		DX12Check(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_DxcCompiler)));
		DX12Check(m_DxcUtils->CreateDefaultIncludeHandler(&m_DxcIncludeHandler));
	}

	DX12ShaderCompiler::~DX12ShaderCompiler()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy Shader Compiler");
		DX12SafeRelease(m_DxcIncludeHandler);
		DX12SafeRelease(m_DxcCompiler);
		DX12SafeRelease(m_DxcUtils);
	}

	IDxcBlobEncoding* DX12ShaderCompiler::LoadShader(std::wstring& fullPath)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Load Shader source");
		VAST_ASSERT(m_DxcUtils);

		// Load shader source file
		IDxcBlobEncoding* sourceBlobEncoding = nullptr;
		DX12Check(m_DxcUtils->LoadFile(fullPath.c_str(), nullptr, &sourceBlobEncoding));
		VAST_ASSERTF(sourceBlobEncoding && sourceBlobEncoding->GetBufferSize(), "Cannot find specified shader path.");

		return sourceBlobEncoding;
	}

	static const wchar_t* ToShaderTarget(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::COMPUTE:
			return L"cs_6_6";
		case ShaderType::VERTEX:
			return L"vs_6_6";
		case ShaderType::PIXEL:
			return L"ps_6_6";
		default:
			VAST_ASSERTF(0, "Shader type not supported.");
			return nullptr;
		}
	}

	IDxcResult* DX12ShaderCompiler::CompileShader(IDxcBlobEncoding* sourceBlobEncoding, const ShaderCompilerArguments& sca)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Compile Shader");
		VAST_ASSERT(sourceBlobEncoding);

		const DxcBuffer sourceBuffer
		{
			.Ptr = sourceBlobEncoding->GetBufferPointer(),
			.Size = sourceBlobEncoding->GetBufferSize(),
			.Encoding = DXC_CP_ACP
		};

		Vector<LPCWSTR> args
		{
			sca.shaderName.c_str(),
			L"-E", sca.shaderEntryPoint.c_str(),
			L"-T", ToShaderTarget(sca.shaderType),
#ifdef VAST_DEBUG
			DXC_ARG_DEBUG,
#else
			DXC_ARG_OPTIMIZATION_LEVEL3,
			//DXC_ARG_SKIP_OPTIMIZATIONS,
#endif
			DXC_ARG_WARNINGS_ARE_ERRORS,
			L"-Qstrip_reflect",
			//L"-Qstrip_debug"
		};

		for (const auto& arg : sca.includeDirectories)
		{
			args.push_back(L"-I");
			args.push_back(arg.c_str());
		}
		
		for (const auto& arg : sca.additionalDefines)
		{
			args.push_back(L"-D");
			args.push_back(arg.c_str());
		}

		// Compile shader
		IDxcResult* compiledShader = nullptr;
		VAST_ASSERT(m_DxcCompiler && m_DxcIncludeHandler);
		DX12Check(m_DxcCompiler->Compile(&sourceBuffer, args.data(), static_cast<uint32>(args.size()), m_DxcIncludeHandler, IID_PPV_ARGS(&compiledShader)));

		// Log compilation errors
		IDxcBlobUtf8* errors = nullptr;
		DX12Check(compiledShader->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
		if (errors && errors->GetStringLength())
		{
			VAST_LOG_CRITICAL("[resource] [shader] Shader compilation error in:\n{}", errors->GetStringPointer());
			return nullptr;
		}
		DX12SafeRelease(errors);

		// Check extra errors
		HRESULT statusResult;
		compiledShader->GetStatus(&statusResult);
		if (FAILED(statusResult))
		{
			VAST_LOG_CRITICAL("[resource] [shader] Shader creation failed.");
			return nullptr;
		}

		return compiledShader;
	}

	ID3D12ShaderReflection* DX12ShaderCompiler::ExtractShaderReflection(IDxcResult* compiledShader)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Extract Shader Reflection");
		VAST_ASSERT(compiledShader && m_DxcUtils);

		IDxcBlob* reflectionBlob = nullptr;
		DX12Check(compiledShader->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr));

		const DxcBuffer reflectionBuffer
		{
			.Ptr = reflectionBlob->GetBufferPointer(),
			.Size = reflectionBlob->GetBufferSize(),
			.Encoding = DXC_CP_ACP
		};

		ID3D12ShaderReflection* shaderReflection = nullptr;
		m_DxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
		DX12SafeRelease(reflectionBlob);
		return shaderReflection;
	}

	IDxcBlob* DX12ShaderCompiler::ExtractShaderOutput(IDxcResult* compiledShader)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Extract Shader Output");
		VAST_ASSERT(compiledShader);

		IDxcBlob* shaderBlob = nullptr;
		compiledShader->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		return shaderBlob;
	}

	IDxcBlob* DX12ShaderCompiler::ExtractShaderPDB(IDxcResult* compiledShader)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Extract Shader PDB");
		VAST_ASSERT(compiledShader);

		IDxcBlob* pdbBlob = nullptr;
		compiledShader->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), nullptr);
		return pdbBlob;
	}

}