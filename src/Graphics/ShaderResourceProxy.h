#pragma once

#include "Core/Core.h"

#include <unordered_map>

namespace vast::gfx
{

	static const uint32 kInvalidShaderResourceProxy = UINT32_MAX;

	struct ShaderResourceProxy
	{
		inline bool IsValid() const { return idx != kInvalidShaderResourceProxy; }
		uint32 idx = kInvalidShaderResourceProxy;
	};

	class ShaderResourceProxyTable
	{
	public:
		ShaderResourceProxyTable(const std::string& shaderName);

		ShaderResourceProxy LookupShaderResource(const std::string& shaderResourceName);

		void Register(const std::string& shaderResourceName, const ShaderResourceProxy& proxyIdx);
		bool IsRegistered(const std::string& shaderResourceName);

	private:
		const std::string m_ShaderName;
		std::unordered_map<std::string, ShaderResourceProxy> m_Map;
	};

}