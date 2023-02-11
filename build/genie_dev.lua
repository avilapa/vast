
PROJ_DIR = path.getabsolute("../projects/dev")

project "dev"
	kind "ConsoleApp" -- WindowedApp for no console
	language "C++"
	
	AddLibrary(spdlog)
	AddLibrary(hlslpp)
	AddLibrary(DirectXAgilitySDK)
	
	files
	{
		path.join(PROJ_DIR, "src/**.h"),
		path.join(PROJ_DIR, "src/**.cpp"),
	}
	
	includedirs
	{
		path.join(PROJ_DIR, "src"),
		path.join(ROOT_DIR, "src"),
		path.join(ROOT_DIR, "vendor"),
	}
	
	links
	{
		"vast", --"d3d12", "dxgi",
	}
	
	configuration "Debug"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Debug/dev"))
		debugdir	(path.join(ROOT_DIR, "build/bin/Debug/dev"))
        objdir 		(path.join(ROOT_DIR, "build/obj/Debug/dev"))
		
	configuration "Release"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Release/dev"))
		objdir 		(path.join(ROOT_DIR, "build/obj/Release/dev"))
	