ROOT_DIR = path.getabsolute("../")
PROJ_DIR = path.getabsolute("../projects/dev")

project "dev"
	kind "ConsoleApp" -- WindowedApp for no console
	language "C++"
	
	files
	{
		path.join(PROJ_DIR, "src/**.h"),
		path.join(PROJ_DIR, "src/**.cpp"),
		path.join(ROOT_DIR, "vendor/hlslpp/include/**.h"),
		path.join(ROOT_DIR, "vendor/spdlog/include/**.h"),
	}
	
	includedirs
	{
		path.join(PROJ_DIR, "src"),
		path.join(ROOT_DIR, "src"),
		path.join(ROOT_DIR, "vendor"),
		path.join(ROOT_DIR, "vendor/hlslpp/include"),
		path.join(ROOT_DIR, "vendor/spdlog/include"),
	}
	
	links
	{
		"vast" 
	}
	
	configuration "Debug"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Debug/dev"))
		debugdir	(path.join(ROOT_DIR, "build/bin/Debug/dev"))
        objdir 		(path.join(ROOT_DIR, "build/obj/Debug/dev"))
		
	configuration "Release"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Release/dev"))
		objdir 		(path.join(ROOT_DIR, "build/obj/Release/dev"))
	