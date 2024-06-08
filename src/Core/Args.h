#pragma once

#include "Core/Defines.h"
#include "Core/Types.h"
#include <unordered_map>

namespace vast
{

	class Arg
	{
	public:
		static bool Init(const std::string& argsFileName);

		Arg(const std::string& name);
		~Arg();

		bool Get();
		bool Get(uint32& v);
		bool Get(uint2& v);
		bool Get(uint3& v);
		bool Get(uint4& v);
		bool Get(float& v);
		bool Get(float2& v);
		bool Get(float3& v);
		bool Get(float4& v);

	private:
		bool m_bInitialized;
		std::string m_Name;
		std::string m_Value;
	};

}