#include "vastpch.h"
#include "Graphics/API/DX12/DX12_ShaderCompiler.h"

#include "dx12/DirectXAgilitySDK/include/d3d12shader.h"
#include "dx12/DirectXShaderCompiler/inc/dxcapi.h"

constexpr wchar_t* SHADER_SOURCE_PATH = L"../../shaders/";
// TODO: Perhaps it makes more sense to move the compiled shaders to the build folder and source to the src folder.
constexpr wchar_t* SHADER_OUTPUT_PATH = L"../../shaders/compiled/";

namespace vast::gfx
{
	DX12ShaderCompiler::DX12ShaderCompiler()
		: m_DxcUtils(nullptr)
		, m_DxcCompiler(nullptr)
		, m_DxcIncludeHandler(nullptr)
	{
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_DxcUtils));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_DxcCompiler));
		m_DxcUtils->CreateDefaultIncludeHandler(&m_DxcIncludeHandler);
	}

	DX12ShaderCompiler::~DX12ShaderCompiler()
	{
		DX12SafeRelease(m_DxcIncludeHandler);
		DX12SafeRelease(m_DxcCompiler);
		DX12SafeRelease(m_DxcUtils);
	}

	void DX12ShaderCompiler::Compile(const ShaderDesc& desc, DX12Shader* outShader)
	{
		VAST_PROFILE_FUNCTION();

		// Load and compile shader
		std::wstring sourcePath;
		sourcePath.append(SHADER_SOURCE_PATH);
		sourcePath.append(desc.shaderName);

		IDxcBlobEncoding* sourceBlobEncoding = nullptr;
		DX12Check(m_DxcUtils->LoadFile(sourcePath.c_str(), nullptr, &sourceBlobEncoding));

		const DxcBuffer sourceBuffer
		{
			sourceBlobEncoding->GetBufferPointer(),
			sourceBlobEncoding->GetBufferSize(),
			DXC_CP_ACP
		};

		std::vector<LPCWSTR> arguments
		{
			desc.shaderName.c_str(),
			L"-E", desc.entryPoint.c_str(),
			L"-T", GetShaderTarget(desc.type),
			L"-I", SHADER_SOURCE_PATH,
#ifdef VAST_DEBUG
			DXC_ARG_DEBUG,
#else
			DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
			DXC_ARG_WARNINGS_ARE_ERRORS,
			L"-Qstrip_reflect",
		};

		IDxcResult* compiledShader = nullptr;
		DX12Check(m_DxcCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32>(arguments.size()), m_DxcIncludeHandler, IID_PPV_ARGS(&compiledShader)));
		DX12SafeRelease(sourceBlobEncoding);

		IDxcBlobUtf8* errors = nullptr;
		DX12Check(compiledShader->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));

		if (errors != nullptr && errors->GetStringLength() != 0)
		{
			VAST_CRITICAL("Shader compilation error in:\n{}", errors->GetStringPointer());
		}
		DX12SafeRelease(errors);

		HRESULT statusResult;
		compiledShader->GetStatus(&statusResult);
		if (FAILED(statusResult))
		{
			VAST_ASSERTF(0, "Shader compilation failed.");
		}

		// Extract reflection
		IDxcBlob* reflectionBlob = nullptr;
		DX12Check(compiledShader->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr));

		const DxcBuffer reflectionBuffer
		{
			reflectionBlob->GetBufferPointer(),
			reflectionBlob->GetBufferSize(),
			DXC_CP_ACP
		};

		ID3D12ShaderReflection* shaderReflection = nullptr;
		m_DxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
		DX12SafeRelease(reflectionBlob);

		// Extract object
		std::wstring dxilPath;
		std::wstring pdbPath;

		dxilPath.append(SHADER_OUTPUT_PATH);
		dxilPath.append(desc.shaderName);
		dxilPath.erase(dxilPath.end() - 5, dxilPath.end());
		dxilPath.append(L".dxil");

		pdbPath = dxilPath;
		pdbPath.append(L".pdb");

		IDxcBlob* shaderBlob = nullptr;
		compiledShader->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		if (shaderBlob != nullptr)
		{
			FILE* fp = nullptr;

			_wfopen_s(&fp, dxilPath.c_str(), L"wb");
			// TODO: Create "compiled" folder if not found, will crash newly cloned builds from repo otherwise.
			VAST_ASSERTF(fp, "Cannot find specified path.");

			fwrite(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 1, fp);
			fclose(fp);
		}

		// Extract debug info
#ifdef VAST_DEBUG
		IDxcBlob* pdbBlob = nullptr;
		DX12Check(compiledShader->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdbBlob), nullptr));
		{
			FILE* fp = nullptr;

			_wfopen_s(&fp, pdbPath.c_str(), L"wb");
			VAST_ASSERTF(fp, "Cannot find specified path.");

			fwrite(pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize(), 1, fp);
			fclose(fp);
		}

		DX12SafeRelease(pdbBlob);
#endif // VAST_DEBUG
		DX12SafeRelease(compiledShader);

		outShader->blob = shaderBlob;
		outShader->reflection = shaderReflection;
	}

	ID3DBlob* DX12ShaderCompiler::CreateRootSignatureFromReflection(DX12Pipeline* pipeline)
	{
		VAST_PROFILE_FUNCTION();
		DX12Shader* shaders[] = { pipeline->vs.get(), pipeline->ps.get() };
		Vector<D3D12_ROOT_PARAMETER1> rootParameters;

		for (auto shader : shaders)
		{
			if (!shader)
				continue;

			D3D12_SHADER_DESC shaderDesc{};
			shader->reflection->GetDesc(&shaderDesc);

			// TODO: Use reflection to deduce InputLayout on non-bindless shaders (e.g. Imgui)
			for (uint32 rscIdx = 0; rscIdx < shaderDesc.BoundResources; ++rscIdx)
			{
				D3D12_SHADER_INPUT_BIND_DESC sibDesc{};
				DX12Check(shader->reflection->GetResourceBindingDesc(rscIdx, &sibDesc));

				if (pipeline->resourceProxyTable.IsRegistered(sibDesc.Name))
					continue;

				pipeline->resourceProxyTable.Register(sibDesc.Name, ShaderResourceProxy{ static_cast<uint32>(rootParameters.size()) });

				if (sibDesc.Type == D3D_SIT_CBUFFER)
				{
					ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = shader->reflection->GetConstantBufferByIndex(rscIdx);
					D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
					shaderReflectionConstantBuffer->GetDesc(&constantBufferDesc);

					D3D12_ROOT_PARAMETER1 rootParameter = {};
					rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					rootParameter.Descriptor.ShaderRegister = sibDesc.BindPoint;
					rootParameter.Descriptor.RegisterSpace = sibDesc.Space;
					rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

					rootParameters.push_back(rootParameter);
				}
			}
		}

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC vrsDesc = {};
		vrsDesc.Desc_1_1.NumParameters = static_cast<uint32>(rootParameters.size());
		vrsDesc.Desc_1_1.pParameters = rootParameters.data();
		vrsDesc.Desc_1_1.NumStaticSamplers = 0;
		vrsDesc.Desc_1_1.pStaticSamplers = nullptr;
		vrsDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
		vrsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

		ID3DBlob* rootSignatureBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		DX12Check(D3D12SerializeVersionedRootSignature(&vrsDesc, &rootSignatureBlob, &errorBlob));
		DX12SafeRelease(errorBlob);
		return rootSignatureBlob;
	}

	const wchar_t* DX12ShaderCompiler::GetShaderTarget(ShaderType type)
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
}