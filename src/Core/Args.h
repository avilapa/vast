#pragma once

#include "Core/Defines.h"
#include "Core/Types.h"

namespace vast
{

	class Arg
	{
	public:
		static bool ParseArgsFile(const std::string& argsFileName);
		static Arg* FindArg(const std::string& argName);

		Arg(const char* name, const std::string& v);
		Arg(const char* name, bool v);
		Arg(const char* name, int32 v);
		Arg(const char* name, int2 v);
		Arg(const char* name, int3 v);
		Arg(const char* name, int4 v);
		Arg(const char* name, uint32 v);
		Arg(const char* name, uint2 v);
		Arg(const char* name, uint3 v);
		Arg(const char* name, uint4 v);
		Arg(const char* name, float v);
		Arg(const char* name, float2 v);
		Arg(const char* name, float3 v);
		Arg(const char* name, float4 v);
		~Arg();

		bool Get();
		bool Get(std::string& v);
		bool Get(std::wstring& v);
		bool Get(bool& v);
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

		std::string GetName() { return m_Name; }
		bool IsString() { return m_bIsString; }

	private:
		Arg() = delete;

		std::string m_Name;
		std::string m_Value;

		bool m_bIsString;
		bool m_bIsInArgsFile;
	};

}