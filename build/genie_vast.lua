ROOT_DIR = path.getabsolute("../")

project "vast"
	kind "StaticLib"
	language "C++"
	
	pchheader "vastpch.h"
	pchsource "../src/vastpch.cpp"
		
	files
	{
		path.join(ROOT_DIR, "src/**.h"),
		path.join(ROOT_DIR, "src/**.cpp"),
		-- Libraries only used within this project also go here (e.g. stb_image)!
		path.join(ROOT_DIR, "vendor/hlslpp/include/**.h"),
		path.join(ROOT_DIR, "vendor/spdlog/include/**.h"),
	}
	
	includedirs
	{
		path.join(ROOT_DIR, "src"),
		path.join(ROOT_DIR, "vendor"),
		path.join(ROOT_DIR, "vendor/spdlog/include"),
		path.join(ROOT_DIR, "vendor/hlslpp/include"),
	}
	
	libdirs
	{
	
	}
	
	links
	{
		"imgui",
	}