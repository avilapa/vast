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

	bool Arg::Get(uint32& v)
	{
		if (!m_Value.empty())
		{
			v = std::stoul(m_Value);
			return true;
		}
		return false;
	}

	static void GetUint(const std::string& argValue, const uint32 count, uint32* v)
	{
		std::istringstream iss(argValue);
		for (uint32 i = 0; i < count; ++i)
		{
			std::string value;
			if (std::getline(iss, value, ','))
			{
				v[i] = std::stoul(value);
			}
			else
			{
				VAST_ASSERTF(0, "Incorrect value format for requested argument.");
			}
		}
	}
	
	bool Arg::Get(uint2& v)
	{
		if (!m_Value.empty())
		{
			std::istringstream iss(m_Value);
			for (uint32 i = 0; i < 2; ++i)
			{
				std::string value;
				if (std::getline(iss, value, ','))
				{
					v[i] = std::stoul(value);
				}
				else
				{
					VAST_ASSERTF(0, "Incorrect value format for requested argument.");
				}
			}

			GetUint(m_Value, 2, (uint32*)v);
			return true;
		}
		return false;
	}
		
	bool Arg::Get(uint3& v)
	{
		if (!m_Value.empty())
		{
			std::istringstream iss(m_Value);
			for (uint32 i = 0; i < 3; ++i)
			{
				std::string value;
				if (std::getline(iss, value, ','))
				{
					v[i] = std::stoul(value);
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
	
	bool Arg::Get(float& v)
	{
		if (!m_Value.empty())
		{
			v = std::stof(m_Value);
			return true;
		}
		return false;
	}

}