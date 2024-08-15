#pragma once

#include "Core/Defines.h"
#include "Core/Types.h"

namespace vast
{

	class Filesystem
	{
	public:
		static std::string GetWorkingDirectory();
		static std::string GetCurrentFilename();

		static bool FileExists(const std::string& filePath);
	};
}