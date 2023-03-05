#include "vastpch.h"
#include "Graphics/ShaderResourceProxy.h"

namespace vast::gfx
{

	ShaderResourceProxy ShaderResourceProxyTable::LookupShaderResource(const std::string& shaderResourceName)
	{
		return map[shaderResourceName];
	}

	void ShaderResourceProxyTable::Register(const std::string& shaderResourceName, const ShaderResourceProxy& proxyIdx)
	{
		if (IsRegistered(shaderResourceName))
		{
			if (proxyIdx.idx == map[shaderResourceName].idx)
			{
				return;
			}
			else
			{
				VAST_WARNING("[resource] Overriding shader resource proxy '{}' value (was '{}', now is '{}'.", shaderResourceName, proxyIdx.idx, map[shaderResourceName].idx);
			}
		}
		map[shaderResourceName] = proxyIdx;
		VAST_INFO("[resource] Registered new shader resource proxy '{}'.", shaderResourceName, proxyIdx.idx);
	}

	bool ShaderResourceProxyTable::IsRegistered(const std::string& shaderResourceName)
	{
		return (map.find(shaderResourceName) != map.end());
	}

}
