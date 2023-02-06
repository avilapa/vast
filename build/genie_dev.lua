ROOT_DIR = path.getabsolute("../")
PROJ_DIR = path.getabsolute("../projects/dev")

project "dev"
	kind "ConsoleApp" -- WindowedApp for no console
	language "C++"
	
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
		"vast" 
	}