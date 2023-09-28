#include "vastpch.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast::gfx
{

	ShaderResourceProxy ShaderResourceProxyTable::LookupShaderResource(const std::string& shaderResourceName)
	{
		return m_Map[shaderResourceName];
	}

	void ShaderResourceProxyTable::Reset()
	{
		m_Map.clear();
	}

	void ShaderResourceProxyTable::Register(const std::string& shaderResourceName, const ShaderResourceProxy& proxyIdx)
	{
#ifdef VAST_DEBUG
		if (IsRegistered(shaderResourceName))
		{
			if (proxyIdx.idx == m_Map[shaderResourceName].idx)
			{
				return;
			}
			else
			{
				VAST_LOG_WARNING("[resource] [shader] Overriding shader resource proxy '{}' value (was '{}', now is '{}'.", shaderResourceName, proxyIdx.idx, m_Map[shaderResourceName].idx);
			}
		}
#endif
		m_Map[shaderResourceName] = proxyIdx;
	}

	bool ShaderResourceProxyTable::IsRegistered(const std::string& shaderResourceName)
	{
		return (m_Map.find(shaderResourceName) != m_Map.end());
	}

}
