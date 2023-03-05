#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <unordered_map>

namespace vast::gfx
{

	static const uint32 kInvalidShaderResourceProxy = UINT32_MAX;

	struct ShaderResourceProxy
	{
		inline bool IsValid() const { return idx != kInvalidShaderResourceProxy; }
		uint32 idx = kInvalidShaderResourceProxy;
	};

	struct ShaderResourceProxyTable
	{
		ShaderResourceProxy LookupShaderResource(const std::string& shaderResourceName);

		void Register(const std::string& shaderResourceName, const ShaderResourceProxy& proxyIdx);
		bool IsRegistered(const std::string& shaderResourceName);

	private:
		std::unordered_map<std::string, ShaderResourceProxy> map;
	};

}