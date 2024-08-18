#include "vastpch.h"
#include "Core/Args.h"

#include "Core/Assert.h"
#include "Core/Log.h"

#include <fstream>
#include <sstream>

namespace vast
{
	// Note: Singleton pattern ensures map is initialized in the global scope before any global Arg
	// attempts to insert into it.
	static std::unordered_map<std::string, Arg*>& GetArgsMap()
	{
		static std::unordered_map<std::string, Arg*> map;
		return map;
	}

	bool Arg::ParseArgsFile(const std::string& argsFileName)
	{
		std::ifstream file(argsFileName);
		if (!VAST_VERIFYF(file.is_open(), "Couldn't find file {}.", argsFileName))
		{
			return false;
		}

		VAST_LOG_INFO("[args] Parsing arguments file:");

		std::string line;
		while (std::getline(file, line))
		{
			// Line has to start with '-' to be a valid argument.
			if (line.empty() || line[0] != '-')
				continue;

			line = line.substr(1);
			std::istringstream iss(line);
			std::string argName;
			// Check if the argument is given a value.
			if (std::getline(iss, argName, '='))
			{
				Arg* arg = FindArg(argName);
				std::string argValue;
				// TODO: We should check the format of the value we're overriding (i.e. is string, is vector...).
				if (arg && std::getline(iss, argValue))
				{
					// If it is a valid argument and provides a proper value, override default value with given value.
					arg->m_Value = argValue;
					arg->m_bIsInArgsFile = true;
					VAST_LOG_INFO("[args]  '-{}={}'", argName, argValue);
				}
				else
				{
					VAST_LOG_WARNING("[args]  '-{}' will be ignored.", line);
				}
			}
			else
			{
				Arg* arg = FindArg(argName);
				if (arg)
				{
					arg->m_Value = "1";
					arg->m_bIsInArgsFile = true;
					VAST_LOG_INFO("[args]  '-{}'", argName);
				}
				else
				{
					VAST_LOG_WARNING("[args]  '-{}' will be ignored.", argName);
				}
			}
		}

		file.close();

		return true;
	}

	Arg* Arg::FindArg(const std::string& argName)
	{
		auto it = GetArgsMap().find(argName);
		if (it != GetArgsMap().end())
		{
			return it->second;
		}
		return nullptr;
	}

	static void AddToArgsMap(Arg* arg)
	{
		VAST_ASSERT(arg);
		if (!GetArgsMap().emplace(arg->GetName(), arg).second)
		{
			VAST_ASSERTF(0, "Arg '{}' already exists in this application (use 'extern' to access it from multiple places).", arg->GetName());
		}
	}

	template<typename T>
	static std::string VectorToString(const T& v, const uint32 n)
	{
		std::string s = std::to_string(v[0]);
		for (uint32 i = 1; i < n; ++i)
		{
			s += std::string("," + std::to_string(v[i]));
		}
		return s;
	}

	Arg::Arg(const char* name, const std::string& v)	: m_Name(name), m_bIsString(true),  m_bIsInArgsFile(false), m_Value(v)						{ AddToArgsMap(this); }
	Arg::Arg(const char* name, bool v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(v ? "1" : "0")			{ AddToArgsMap(this); }
	Arg::Arg(const char* name, int32 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(std::to_string(v))		{ AddToArgsMap(this); }
	Arg::Arg(const char* name, int2 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 2))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, int3 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 3))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, int4 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 4))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, uint32 v)				: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(std::to_string(v))		{ AddToArgsMap(this); }
	Arg::Arg(const char* name, uint2 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 2))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, uint3 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 3))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, uint4 v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 4))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, float v)					: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(std::to_string(v))		{ AddToArgsMap(this); }
	Arg::Arg(const char* name, float2 v)				: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 2))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, float3 v)				: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 3))	{ AddToArgsMap(this); }
	Arg::Arg(const char* name, float4 v)				: m_Name(name), m_bIsString(false), m_bIsInArgsFile(false), m_Value(VectorToString(v, 4))	{ AddToArgsMap(this); }

	Arg::~Arg()
	{
		GetArgsMap().erase(m_Name);
	}

	bool Arg::Get()
	{
		return m_bIsInArgsFile;
	}

	bool Arg::Get(std::string& v)
	{
		v = m_Value;
		return m_bIsInArgsFile;
	}

	bool Arg::Get(std::wstring& v)
	{
		v = std::wstring(m_Value.begin(), m_Value.end());
		return m_bIsInArgsFile;
	}

	template<typename T>
	static constexpr T StringTo(const std::string& value) 
	{
		if constexpr (std::is_same_v<T, uint32>) 
		{
			return static_cast<uint32>(std::stoul(value));
		}
		else if constexpr (std::is_same_v<T, int32>) 
		{
			return static_cast<int32>(std::stoi(value));
		}
		else if constexpr (std::is_same_v<T, float>) 
		{
			return std::stof(value);
		}
		else 
		{
			static_assert(std::is_same_v<T, void>, "Unsupported type");
		}
	}

	bool Arg::Get(bool& v)
	{
		VAST_ASSERTF(!m_bIsString && !m_Value.empty(), "Unable to retrieve numeric value.");
		uint32 num = StringTo<uint32>(m_Value);
		VAST_ASSERT(v == 0 || v == 1);
		v = num;
		return m_bIsInArgsFile;
	}

	bool Arg::Get(int32& v)
	{
		VAST_ASSERTF(!m_bIsString && !m_Value.empty(), "Unable to retrieve numeric value.");
		v = StringTo<int32>(m_Value);
		return m_bIsInArgsFile;
	}
	
	bool Arg::Get(uint32& v)
	{
		VAST_ASSERTF(!m_bIsString && !m_Value.empty(), "Unable to retrieve numeric value.");
		v = StringTo<uint32>(m_Value);
		return m_bIsInArgsFile;
	}
	
	bool Arg::Get(float& v)
	{
		VAST_ASSERTF(!m_bIsString && !m_Value.empty(), "Unable to retrieve numeric value.");
		v = StringTo<float>(m_Value);
		return m_bIsInArgsFile;
	}

	template<typename T>
	static void GetVector(Arg* arg, const uint32 n, T& v)
	{
		std::string argValue;
		arg->Get(argValue);

		VAST_ASSERTF(!arg->IsString() && !argValue.empty(), "Unable to retrieve numeric value.");

		std::istringstream iss(argValue);
		for (uint32 i = 0; i < n; ++i)
		{
			std::string tokenValue;
			if (std::getline(iss, tokenValue, ','))
			{
				v[i] = StringTo<std::remove_reference_t<decltype(v[i])>>(tokenValue);
			}
			else
			{
				VAST_ASSERTF(0, "Incorrect value format for requested argument.");
			}
		}
	}

	bool Arg::Get(int2& v)		{ GetVector(this, 2, v); return m_bIsInArgsFile; }
	bool Arg::Get(int3& v)		{ GetVector(this, 3, v); return m_bIsInArgsFile; }
	bool Arg::Get(int4& v)		{ GetVector(this, 4, v); return m_bIsInArgsFile; }

	bool Arg::Get(uint2& v)		{ GetVector(this, 2, v); return m_bIsInArgsFile; }
	bool Arg::Get(uint3& v)		{ GetVector(this, 3, v); return m_bIsInArgsFile; }
	bool Arg::Get(uint4& v)		{ GetVector(this, 4, v); return m_bIsInArgsFile; }

	bool Arg::Get(float2& v)	{ GetVector(this, 2, v); return m_bIsInArgsFile; }
	bool Arg::Get(float3& v)	{ GetVector(this, 3, v); return m_bIsInArgsFile; }
	bool Arg::Get(float4& v)	{ GetVector(this, 4, v); return m_bIsInArgsFile; }

}