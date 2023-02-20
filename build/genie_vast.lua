
project "vast"
	kind "StaticLib"
	language "C++"
	
	pchheader "vastpch.h"
	pchsource "../src/vastpch.cpp"
	
	AddLibrary(spdlog)
	AddLibrary(hlslpp)
	AddDX12Libraries()
	
	files
	{
		path.join(ROOT_DIR, "src/**.h"),
		path.join(ROOT_DIR, "src/**.cpp"),
		path.join(ROOT_DIR, "shaders/**.h"),
	}
	
	includedirs
	{
		path.join(ROOT_DIR, "src"),
		path.join(ROOT_DIR, "shaders"),
		path.join(ROOT_DIR, "vendor"),
	}

	links {	"imgui" }
	
	configuration "Debug"
		links { "minitrace" }
		defines { "MTR_ENABLED"	}
	