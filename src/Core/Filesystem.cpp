#include "vastpch.h"
#include "Core/Filesystem.h"

#include <filesystem>

namespace vast
{

	namespace Filesystem
	{

		std::string GetWorkingDirectory()
		{
			return std::filesystem::current_path().string();
		}

		std::string GetCurrentFilename()
		{
			return std::filesystem::path(__FILE__).filename().string();
		}

		bool FileExists(const std::string& filePath)
		{
			return std::filesystem::exists(filePath);
		}

	}

}
