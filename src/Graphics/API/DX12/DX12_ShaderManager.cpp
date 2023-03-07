#include "vastpch.h"
#include "Graphics/API/DX12/DX12_ShaderManager.h"

#include "dx12/DirectXShaderCompiler/inc/dxcapi.h"

constexpr wchar_t* SHADER_SOURCE_PATH = L"../../shaders/";
// TODO: Perhaps it makes more sense to move the compiled shaders to the build folder and source to the src folder.
constexpr wchar_t* SHADER_OUTPUT_PATH = L"../../shaders/compiled/";

// Note: Root 32 Bit constants are identified on shaders by using a reserved binding point b999.
// This is because DXC shader reflection has no way to tell apart a CBV from a Root 32 Bit Constant.
// TODO: We could also identify push constants by giving a descriptive name to the buffer itself, in case in the future more than one binding point is needed.
static constexpr UINT PUSH_CONSTANT_REGISTER_INDEX = 999;

namespace vast::gfx
{
	DX12ShaderManager::DX12ShaderManager()
		: m_DxcUtils(nullptr)
		, m_DxcCompiler(nullptr)
		, m_DxcIncludeHandler(nullptr)
	{
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_DxcUtils));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_DxcCompiler));
		m_DxcUtils->CreateDefaultIncludeHandler(&m_DxcIncludeHandler);
	}

	DX12ShaderManager::~DX12ShaderManager()
	{
		for (auto shader : m_Shaders)
		{
			DX12SafeRelease(shader->blob);
			DX12SafeRelease(shader->reflection);
			shader = nullptr; // TODO: Make sure all shaders are deleted here (pass weak ptr instead of ref?)
		}

		DX12SafeRelease(m_DxcIncludeHandler);
		DX12SafeRelease(m_DxcCompiler);
		DX12SafeRelease(m_DxcUtils);
	}

	Ref<DX12Shader> DX12ShaderManager::LoadShader(const ShaderDesc& desc)
	{
		Ref<DX12Shader> shaderRef;
 		auto key = MakeShaderKey(desc.shaderName, desc.entryPoint);
		if (IsShaderRegistered(key))
		{
			shaderRef = m_Shaders[m_ShaderKeys[key]];
		}
		else
		{
			shaderRef = CompileShader(desc, key);
		}

		VAST_ASSERT(shaderRef);
		VAST_ASSERT(shaderRef->blob);
		VAST_ASSERT(shaderRef->reflection);
		return shaderRef;
	}

	std::string DX12ShaderManager::MakeShaderKey(const std::string& shaderName, const std::string& entryPoint) const
	{
		// TODO: This could be a hash.
		return shaderName + entryPoint;
	}

	bool DX12ShaderManager::IsShaderRegistered(const std::string key) const
	{
		return (m_ShaderKeys.find(key) != m_ShaderKeys.end());
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

	Ref<DX12Shader> DX12ShaderManager::CompileShader(const ShaderDesc& desc, const std::string& key)
	{
		VAST_PROFILE_FUNCTION();
		std::wstring shaderName(desc.shaderName.begin(), desc.shaderName.end());
		std::wstring entryPoint(desc.entryPoint.begin(), desc.entryPoint.end());

		// Load and compile shader
		std::wstring sourcePath = SHADER_SOURCE_PATH + shaderName;
		IDxcBlobEncoding* sourceBlobEncoding = nullptr;
		DX12Check(m_DxcUtils->LoadFile(sourcePath.c_str(), nullptr, &sourceBlobEncoding));

		const DxcBuffer sourceBuffer
		{
			sourceBlobEncoding->GetBufferPointer(),
			sourceBlobEncoding->GetBufferSize(),
			DXC_CP_ACP
		};

		Vector<LPCWSTR> arguments
		{
			shaderName.c_str(),
			L"-E", entryPoint.c_str(),
			L"-T", ToShaderTarget(desc.type),
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
		std::wstring dxilPath = SHADER_OUTPUT_PATH + shaderName;
		dxilPath.erase(dxilPath.end() - 5, dxilPath.end());
		dxilPath.append(L".dxil");
		std::wstring pdbPath = dxilPath;
		pdbPath.append(L".pdb"); // TODO: dxil.pdb?

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

		Ref<DX12Shader> shaderRef = MakeRef<DX12Shader>();
		shaderRef->blob = shaderBlob;
		shaderRef->reflection = shaderReflection;

		m_ShaderKeys[key] = static_cast<uint32>(m_Shaders.size());
		m_Shaders.push_back(shaderRef);

		return shaderRef;
	}

	ID3DBlob* DX12ShaderManager::CreateRootSignatureFromReflection(DX12Pipeline* pipeline) const
	{
		VAST_PROFILE_FUNCTION();
		DX12Shader* shaders[] = { pipeline->vs.get(), pipeline->ps.get() };
		Vector<D3D12_ROOT_PARAMETER1> rootParameters;
		Vector<D3D12_DESCRIPTOR_RANGE1> descriptorRanges;

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

				if (pipeline->resourceProxyTable->IsRegistered(sibDesc.Name))
					continue;

				pipeline->resourceProxyTable->Register(sibDesc.Name, ShaderResourceProxy{ static_cast<uint32>(rootParameters.size()) });

				if (sibDesc.Type == D3D_SIT_CBUFFER)
				{
					ID3D12ShaderReflectionConstantBuffer* shaderReflectionCB = shader->reflection->GetConstantBufferByIndex(rscIdx);
					D3D12_SHADER_BUFFER_DESC cbDesc{};
					shaderReflectionCB->GetDesc(&cbDesc);

					D3D12_ROOT_PARAMETER1 rootParameter = {};

					if (sibDesc.BindPoint == PUSH_CONSTANT_REGISTER_INDEX)
					{
						rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
						rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
						rootParameter.Constants.ShaderRegister = sibDesc.BindPoint;
						rootParameter.Constants.RegisterSpace = sibDesc.Space;
						rootParameter.Constants.Num32BitValues = cbDesc.Size / 4;
					}
					else
					{
						rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
						rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
						rootParameter.Descriptor.ShaderRegister = sibDesc.BindPoint;
						rootParameter.Descriptor.RegisterSpace = sibDesc.Space;
						rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
					}

					rootParameters.push_back(rootParameter);
				}
				else if (sibDesc.Type == D3D_SIT_TEXTURE)
				{
					D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
					descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					descriptorRange.NumDescriptors = 1;
					descriptorRange.BaseShaderRegister = sibDesc.BindPoint;
					descriptorRange.RegisterSpace = sibDesc.Space;
					// TODO: Review usage, ideally we can set D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC for most stuff.
					descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
					descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32>(descriptorRanges.size());

					descriptorRanges.push_back(descriptorRange);
				}
			}

			if (!descriptorRanges.empty())
			{
				D3D12_ROOT_PARAMETER1 descriptorTable = {};
				descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32>(descriptorRanges.size());
				descriptorTable.DescriptorTable.pDescriptorRanges = descriptorRanges.data();

				rootParameters.push_back(descriptorTable);
			}
		}

		// TODO: This is required by ImGui, but not necessary for all pipelines.
		D3D12_STATIC_SAMPLER_DESC staticSampler = {};
		staticSampler.Filter			= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias		= 0.0f;
		staticSampler.MaxAnisotropy		= 0;
		staticSampler.ComparisonFunc	= D3D12_COMPARISON_FUNC_ALWAYS;
		staticSampler.BorderColor		= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampler.MinLOD			= 0.0f;
		staticSampler.MaxLOD			= 0.0f;
		staticSampler.ShaderRegister	= 0;
		staticSampler.RegisterSpace		= 0;
		staticSampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters		= static_cast<uint32>(rootParameters.size());
		rootSignatureDesc.pParameters		= rootParameters.data();
		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers	= &staticSampler;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT // TODO: Remove this flag from bindless pipelines (minor optimization)
								| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
								| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
								| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
								| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
								| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignature = {};
		versionedRootSignature.Desc_1_1 = rootSignatureDesc;
		versionedRootSignature.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

		ID3DBlob* rootSignatureBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		DX12Check(D3D12SerializeVersionedRootSignature(&versionedRootSignature, &rootSignatureBlob, &errorBlob));
		DX12SafeRelease(errorBlob);
		return rootSignatureBlob;
	}
	
	Vector<D3D12_SIGNATURE_PARAMETER_DESC> DX12ShaderManager::GetInputParametersFromReflection(ID3D12ShaderReflection* reflection) const
	{
		VAST_ASSERT(reflection);

		D3D12_SHADER_DESC shaderDesc{};
		reflection->GetDesc(&shaderDesc);

		Vector<D3D12_SIGNATURE_PARAMETER_DESC> params;
		params.reserve(shaderDesc.InputParameters);

		for (uint32 i = 0; i < shaderDesc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
			reflection->GetInputParameterDesc(i, &paramDesc);
			params.emplace_back(paramDesc);
		}

		return params;
	}


}