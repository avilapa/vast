#include "vastpch.h"
#include "Core/Args.h"

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

	static Arg* FindArg(const std::string& argName)
	{
		auto it = GetArgsMap().find(argName);
		if (it != GetArgsMap().end())
		{
			return it->second;
		}
		return nullptr;
	}

	bool Arg::Init(const std::string& argsFileName)
	{
		std::ifstream file(argsFileName);
		if (!VAST_VERIFYF(file.is_open(), "Couldn't find file {}.", argsFileName))
		{
			return false;
		}

		VAST_LOG_INFO("[args] Parsing arguments file '{}':");

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] != '-')
				continue;

			std::istringstream iss(line.substr(1));
			std::string argName;
			if (std::getline(iss, argName, '='))
			{
				std::string argValue;
				if (std::getline(iss, argValue))
				{
					Arg* arg = FindArg(argName);
					if (arg)
					{
						arg->m_Value = argValue;
						VAST_LOG_INFO("[args]  '-{}={}'", argName, argValue);
					}
					else
					{
						VAST_LOG_WARNING("[args]  '-{}' will be ignored.", argName);
					}
				}
			}
			else
			{
				Arg* arg = FindArg(argName);
				if (arg)
				{
					arg->m_Value = "1";
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

	Arg::Arg(const std::string& name)
		: m_bInitialized(false)
		, m_Name(name)
		, m_Value()
	{
		auto result = GetArgsMap().emplace(m_Name, this);

		m_bInitialized = VAST_VERIFYF(result.second, "Arg '{}' already exists in this application (use 'extern' to access it from multiple places).", m_Name);
	}

	Arg::~Arg()
	{
		if (m_bInitialized)
		{
			GetArgsMap().erase(m_Name);
		}
	}

	bool Arg::Get()
	{
		if (!m_Value.empty())
		{
			return m_Value == "1";
		}
		return false;
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

	bool Arg::Get(int32& v)
	{
		if (!m_Value.empty())
		{
			v = StringTo<int32>(m_Value);
			return true;
		}
		return false;
	}
	
	bool Arg::Get(uint32& v)
	{
		if (!m_Value.empty())
		{
			v = StringTo<uint32>(m_Value);
			return true;
		}
		return false;
	}
	
	bool Arg::Get(float& v)
	{
		if (!m_Value.empty())
		{
			v = StringTo<float>(m_Value);
			return true;
		}
		return false;
	}

	template<typename T>
	static bool GetVector(const std::string& argValues, const uint32 n, T& v)
	{
		if (!argValues.empty())
		{
			std::istringstream iss(argValues);
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
			return true;
		}
		return false;
	}

	bool Arg::Get(int2& v)		{ return GetVector(m_Value, 2, v); }
	bool Arg::Get(int3& v)		{ return GetVector(m_Value, 3, v); }
	bool Arg::Get(int4& v)		{ return GetVector(m_Value, 4, v); }

	bool Arg::Get(uint2& v)		{ return GetVector(m_Value, 2, v); }
	bool Arg::Get(uint3& v)		{ return GetVector(m_Value, 3, v); }
	bool Arg::Get(uint4& v)		{ return GetVector(m_Value, 4, v); }

	bool Arg::Get(float2& v)	{ return GetVector(m_Value, 2, v); }
	bool Arg::Get(float3& v)	{ return GetVector(m_Value, 3, v); }
	bool Arg::Get(float4& v)	{ return GetVector(m_Value, 4, v); }

}