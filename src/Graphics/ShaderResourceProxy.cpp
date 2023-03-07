#include "vastpch.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast::gfx
{

	ShaderResourceProxyTable::ShaderResourceProxyTable(const std::string& shaderName)
		: m_ShaderName(shaderName)
	{

	}

	ShaderResourceProxy ShaderResourceProxyTable::LookupShaderResource(const std::string& shaderResourceName)
	{
		return m_Map[shaderResourceName];
	}

	void ShaderResourceProxyTable::Register(const std::string& shaderResourceName, const ShaderResourceProxy& proxyIdx)
	{
		if (IsRegistered(shaderResourceName))
		{
			if (proxyIdx.idx == m_Map[shaderResourceName].idx)
			{
				return;
			}
			else
			{
				VAST_WARNING("[resource] Overriding shader resource proxy '{}' value (was '{}', now is '{}'.", shaderResourceName, proxyIdx.idx, m_Map[shaderResourceName].idx);
			}
		}
		m_Map[shaderResourceName] = proxyIdx;
		VAST_INFO("[resource] Registered new shader resource proxy '{}' in {}.", shaderResourceName, m_ShaderName);
	}

	bool ShaderResourceProxyTable::IsRegistered(const std::string& shaderResourceName)
	{
		return (m_Map.find(shaderResourceName) != m_Map.end());
	}

}
