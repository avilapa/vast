
PROJ_DIR = path.getabsolute("../")

project "samples"
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
		path.join(ROOT_DIR, "shaders"),
		path.join(ROOT_DIR, "vendor"),
	}
	
	links {	"vast" }
	
	configuration "Debug"
		targetdir 	(path.join(PROJ_DIR, "build/bin/Debug/"))
        objdir 		(path.join(PROJ_DIR, "build/obj/Debug/"))
		CopyDLLs(PROJ_DIR)
		
	configuration "Release"
		targetdir 	(path.join(PROJ_DIR, "build/bin/Release/"))
		objdir 		(path.join(PROJ_DIR, "build/obj/Release/"))
		CopyDLLs(PROJ_DIR)
	