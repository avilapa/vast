
PROJ_DIR = path.getabsolute("../")

project "dev"
	kind "ConsoleApp" -- WindowedApp for no console
	language "C++"
	
	AddLibrary(spdlog)
	AddLibrary(hlslpp)
	AddDX12Libraries()
	
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
	
	links {	"vast" }
	
	debugargs { path.join(PROJ_DIR, "build/args.txt") }
	
	configuration "Debug"
		targetdir 	(path.join(PROJ_DIR, "build/bin/Debug/"))
        objdir 		(path.join(PROJ_DIR, "build/obj/Debug/"))
		CopyDebugDLLs(PROJ_DIR)
		
	configuration "Release"
		targetdir 	(path.join(PROJ_DIR, "build/bin/Release/"))
		objdir 		(path.join(PROJ_DIR, "build/obj/Release/"))
		CopyReleaseDLLs(PROJ_DIR)
	