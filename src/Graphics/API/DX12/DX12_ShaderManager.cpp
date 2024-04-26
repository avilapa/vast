#include "vastpch.h"
#include "Graphics/API/DX12/DX12_ShaderManager.h"
#include "Graphics/API/DX12/DX12_ShaderCompiler.h"

// TODO: DX12ShaderManager shouldn't have to include this or be aware of compiler specific arguments, should be moved down to DX12ShaderCompiler.
#include "dx12/DirectXShaderCompiler/inc/dxcapi.h"
#include "dx12/DirectXAgilitySDK/include/d3d12shader.h"

static const wchar_t* SHADER_SOURCE_PATH = L"../../shaders/";
// TODO: Perhaps it makes more sense to move the compiled shaders to the build folder and source to the src folder.
static const wchar_t* SHADER_OUTPUT_PATH = L"../../shaders/compiled/";

// Note: Root 32 Bit constants are identified on shaders by using a reserved binding point b999.
// This is because DXC shader reflection has no way to tell apart a CBV from a Root 32 Bit Constant.
// TODO: We could also identify push constants by giving a descriptive name to the buffer itself, in case in the future more than one binding point is needed.
static constexpr int PUSH_CONSTANT_REGISTER_INDEX = 999;

namespace vast::gfx
{

	DX12ShaderManager::DX12ShaderManager()
		: m_ShaderCompiler(nullptr)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Create Shader Manager");
		m_ShaderCompiler = MakePtr<DX12ShaderCompiler>();

		m_SharedCompilerArgs =
		{
			L"-I", SHADER_SOURCE_PATH,
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

		AddGlobalShaderDefine("PushConstantRegister", std::string("b") + std::to_string(PUSH_CONSTANT_REGISTER_INDEX));
	}

	DX12ShaderManager::~DX12ShaderManager()
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Destroy Shader Manager");
		for (auto shader : m_Shaders)
		{
			DX12SafeRelease(shader.first->blob);
			DX12SafeRelease(shader.first->reflection);
			shader.first = nullptr; // TODO: Make sure all shaders are deleted here (pass weak ptr instead of ref?)
		}
		m_Shaders.clear();

		m_ShaderCompiler = nullptr;
	}

	// From: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
	static std::wstring StringToWString(const std::string& s, bool bIsUTF8 = true)
	{
		int32 slength = (int32)s.length() + 1;
		int32 len = MultiByteToWideChar(bIsUTF8 ? CP_UTF8 : CP_ACP, 0, s.c_str(), slength, 0, 0);
		std::wstring buf;
		buf.resize(len);
		MultiByteToWideChar(bIsUTF8 ? CP_UTF8 : CP_ACP, 0, s.c_str(), slength, const_cast<wchar_t*>(buf.c_str()), len);
		return buf;
	}

	void DX12ShaderManager::AddGlobalShaderDefine(const std::string& name, const std::string& value)
	{
		m_SharedCompilerArgs.push_back(L"-D");
		m_SharedCompilerArgs.push_back(StringToWString(name + "=" + value));
	}

	Ref<DX12Shader> DX12ShaderManager::LoadShader(const ShaderDesc& desc)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Load Shader");
		Ref<DX12Shader> shaderRef;
 		auto key = MakeShaderKey(desc.shaderName, desc.entryPoint);
		if (IsShaderRegistered(key))
		{
			shaderRef = m_Shaders[m_ShaderKeys[key]].first;
		}
		else
		{
			shaderRef = MakeRef<DX12Shader>();
			shaderRef->key = key;

			bool success = false;
			while (!success)
			{
				VAST_LOG_INFO("[resource] [shader] Compiling new shader '{}' with entry point '{}'", desc.shaderName, desc.entryPoint);
				// If we get a shader compile error on startup, allow the user to fix the issue and continue launching the application.
				success = CompileShader(desc, shaderRef.get());
				VAST_ASSERTF(success, "Shader Compilation Failed.");
			}

			m_ShaderKeys[key] = static_cast<uint32>(m_Shaders.size());
			m_Shaders.push_back({ shaderRef, desc });
		}

		VAST_ASSERT(shaderRef);
		VAST_ASSERT(shaderRef->blob);
		VAST_ASSERT(shaderRef->reflection);
		return shaderRef;
	}

	bool DX12ShaderManager::ReloadShader(Ref<DX12Shader> shader)
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Reload Shader");
		VAST_ASSERTF(IsShaderRegistered(shader->key), "Attempted to reload invalid shader.");
		auto desc = m_Shaders[m_ShaderKeys[shader->key]].second;
		VAST_ASSERT(shader->key.compare(MakeShaderKey(desc.shaderName, desc.entryPoint)) == 0);
		if (CompileShader(desc, shader.get()))
		{
			VAST_LOG_TRACE("[resource] [shader] Reloaded shader '{}' with entry point '{}'.", desc.shaderName, desc.entryPoint);
			return true;
		}
		VAST_LOG_WARNING("[resource] [shader] Failed to reload shader '{}' with entry point '{}' due to a compile error.", desc.shaderName, desc.entryPoint);
		return false;
	}

	std::string DX12ShaderManager::MakeShaderKey(const std::string& shaderName, const std::string& entryPoint) const
	{
		// TODO: This could be a hash.
		return shaderName + ',' + entryPoint;
	}

	bool DX12ShaderManager::IsShaderRegistered(const std::string key) const
	{
		return (m_ShaderKeys.find(key) != m_ShaderKeys.end());
	}

	bool DX12ShaderManager::CompileShader(const ShaderDesc& desc, DX12Shader* outShader)
	{
		std::wstring shaderName(desc.shaderName.begin(), desc.shaderName.end());
		std::wstring entryPoint(desc.entryPoint.begin(), desc.entryPoint.end());
		std::wstring fullPath = SHADER_SOURCE_PATH + shaderName;

		IDxcBlobEncoding* sourceBlobEncoding = m_ShaderCompiler->LoadShader(fullPath);

		Vector<LPCWSTR> args
		{
			shaderName.c_str(),										// Optional source file name used in error logging.
			L"-E", entryPoint.c_str(),								// Entry point function name.
			L"-T", DX12ShaderCompiler::ToShaderTarget(desc.type),	// Shader target profile.
		};

		for (auto& arg : m_SharedCompilerArgs)
		{
			args.push_back(arg.c_str());
		}

		IDxcResult* compiledShader = m_ShaderCompiler->CompileShader(sourceBlobEncoding, args);

		ID3D12ShaderReflection* shaderReflection = m_ShaderCompiler->ExtractShaderReflection(compiledShader);
		IDxcBlob* shaderBlob = m_ShaderCompiler->ExtractShaderOutput(compiledShader);
		// TODO: Write out DXIL?
// 		{
// 			std::wstring dxilPath = SHADER_OUTPUT_PATH + shaderName;
// 			dxilPath.erase(dxilPath.end() - 5, dxilPath.end());
// 			dxilPath.append(L".dxil"); // .bin?
// 
// 			FILE* fp = nullptr;
// 			_wfopen_s(&fp, dxilPath.c_str(), L"wb");
// 			VAST_ASSERTF(fp, "Cannot find specified path.");
// 			fwrite(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 1, fp);
// 			fclose(fp);
// 		}
#ifdef VAST_DEBUG
		IDxcBlob* shaderDebugBlob = m_ShaderCompiler->ExtractShaderPDB(compiledShader);
		// TODO: Write out PDB?
// 		{
// 			std::wstring pdbPath = SHADER_OUTPUT_PATH + shaderName;
// 			pdbPath.erase(pdbPath.end() - 5, pdbPath.end());
// 			pdbPath.append(L".pdb");
// 
// 			FILE* fp = nullptr;
// 			_wfopen_s(&fp, pdbPath.c_str(), L"wb");
// 			VAST_ASSERTF(fp, "Cannot find specified path.");
// 			fwrite(pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize(), 1, fp);
// 			fclose(fp);
// 		}
		DX12SafeRelease(shaderDebugBlob);
#endif

		DX12SafeRelease(sourceBlobEncoding);
		DX12SafeRelease(compiledShader);

		DX12SafeRelease(outShader->blob);
		DX12SafeRelease(outShader->reflection);
		outShader->blob = shaderBlob;
		outShader->reflection = shaderReflection;
		return true;
	}

	ID3DBlob* DX12ShaderManager::CreateRootSignatureFromReflection(DX12Pipeline& pipeline) const
	{
		VAST_PROFILE_TRACE_SCOPE("gfx", "Reflect Root Signature");
		DX12Shader* shaders[] = { pipeline.vs.get(), pipeline.ps.get(), pipeline.cs.get() };
		Vector<D3D12_ROOT_PARAMETER1> rootParameters;
		Vector<D3D12_DESCRIPTOR_RANGE1> descriptorRanges;

		for (auto shader : shaders)
		{
			if (!shader)
				continue;

			D3D12_SHADER_DESC shaderDesc{};
			shader->reflection->GetDesc(&shaderDesc);

			for (uint32 rscIdx = 0; rscIdx < shaderDesc.BoundResources; ++rscIdx)
			{
				D3D12_SHADER_INPUT_BIND_DESC sibDesc{};
				DX12Check(shader->reflection->GetResourceBindingDesc(rscIdx, &sibDesc));

				// Skip resource if it was already registered on a previous shader of this pipeline.
				if (pipeline.resourceProxyTable.IsRegistered(sibDesc.Name))
					continue;

				pipeline.resourceProxyTable.Register(sibDesc.Name, ShaderResourceProxy{ static_cast<uint32>(rootParameters.size()) });
				VAST_LOG_TRACE("[resource] [shader] Registered shader resource '{}'.", sibDesc.Name);

				switch (sibDesc.Type)
				{
				case D3D_SIT_CBUFFER:
				{
					// CBV or PushConstants
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

						VAST_ASSERTF(pipeline.pushConstantIndex == UINT8_MAX, "Multiple push constants for a single pipeline not currently supported.");
						pipeline.pushConstantIndex = static_cast<uint8>(rootParameters.size());
					}
					else
					{
						rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
						rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
						rootParameter.Descriptor.ShaderRegister = sibDesc.BindPoint;
						rootParameter.Descriptor.RegisterSpace = sibDesc.Space;
						// TODO: Review usage, ideally we can set D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC for most stuff.
						rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
					}

					rootParameters.push_back(rootParameter);
					break;
				}
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_TEXTURE:
				{
					// SRV
					D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
					descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					descriptorRange.NumDescriptors = 1;
					descriptorRange.BaseShaderRegister = sibDesc.BindPoint;
					descriptorRange.RegisterSpace = sibDesc.Space;
					// TODO: Review usage, ideally we can set D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC for most stuff.
					descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
					descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32>(descriptorRanges.size());

					descriptorRanges.push_back(descriptorRange);
					break;
				}
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWTYPED:
				{
					// UAV
					D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
					descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
					descriptorRange.NumDescriptors = 1;
					descriptorRange.BaseShaderRegister = sibDesc.BindPoint;
					descriptorRange.RegisterSpace = sibDesc.Space;
					// TODO: Review usage, ideally we can set D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC for most stuff.
					descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
					descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32>(descriptorRanges.size());

					descriptorRanges.push_back(descriptorRange);
					break;
				}
				case D3D_SIT_SAMPLER:
					break; // TODO: Should we assert here? Test using one of these
				default:
					VAST_ASSERTF(0, "Shader Input type not currently supported.");
				}
			}
		}

		if (!descriptorRanges.empty())
		{
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32>(descriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = descriptorRanges.data();

			VAST_ASSERTF(pipeline.descriptorTableIndex == UINT8_MAX, "Multiple descriptor tables for a single pipeline not currently supported.");
			pipeline.descriptorTableIndex = static_cast<uint8>(rootParameters.size());
			rootParameters.push_back(descriptorTable);
		}

		D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
		rootSignatureDesc.NumParameters		= static_cast<uint32>(rootParameters.size());
		rootSignatureDesc.pParameters		= rootParameters.data();
		rootSignatureDesc.NumStaticSamplers = 0;
		rootSignatureDesc.pStaticSamplers	= nullptr;
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