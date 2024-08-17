#pragma once

#include "Core/Defines.h"

#include <string>

namespace vast
{

	namespace Filesystem
	{
		std::string GetWorkingDirectory();
		std::string GetCurrentFilename();

		bool FileExists(const std::string& filePath);
	}
}