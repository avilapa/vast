
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
		path.join(ROOT_DIR, "shaders/**.hlsl"),
		path.join(ROOT_DIR, "shaders/**.hlsli"),
	}
	
	includedirs
	{
		path.join(ROOT_DIR, "src"),
		path.join(ROOT_DIR, "shaders"),
		path.join(ROOT_DIR, "vendor"),
		path.join(ROOT_DIR, "vendor/imgui"), -- Only necessary while we use Imgui stock implementation on win32
	}

	links {	"imgui" }
	
	configuration "Debug"
		links { "minitrace" }
		defines { "MTR_ENABLED"	}
	