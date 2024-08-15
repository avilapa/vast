#include "vastpch.h"
#include "Core/Filesystem.h"

#include <filesystem>

namespace vast
{
	
	std::string Filesystem::GetWorkingDirectory()
	{
		return std::filesystem::current_path().string();
	}

	std::string Filesystem::GetCurrentFilename()
	{
		return std::filesystem::path(__FILE__).filename().string();
	}

	bool Filesystem::FileExists(const std::string& filePath)
	{
		return std::filesystem::exists(filePath);
	}

}
