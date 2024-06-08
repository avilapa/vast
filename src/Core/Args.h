#pragma once

#include "Core/Defines.h"
#include "Core/Types.h"

namespace vast
{

	class Arg
	{
	public:
		static bool Init(const std::string& argsFileName);

		Arg(const std::string& name);
		~Arg();

		bool Get();
		bool Get(std::string& v);
		bool Get(int32& v);
		bool Get(int2& v);
		bool Get(int3& v);
		bool Get(int4& v);
		bool Get(uint32& v);
		bool Get(uint2& v);
		bool Get(uint3& v);
		bool Get(uint4& v);
		bool Get(float& v);
		bool Get(float2& v);
		bool Get(float3& v);
		bool Get(float4& v);

	private:
		Arg() = delete;

		bool m_bInitialized;
		std::string m_Name;
		std::string m_Value;
	};

}