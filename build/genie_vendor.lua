ROOT_DIR = path.getabsolute("../")
IMGUI_DIR = path.getabsolute("../vendor/imgui")

project "imgui"
	kind "StaticLib"
	language "C++"
	
	files
	{
		path.join(IMGUI_DIR, "im*.h"),
		path.join(IMGUI_DIR, "im*.cpp"),
	}

project "minitrace"
	kind "StaticLib"
	language "C"
	
	files
	{
		path.join(ROOT_DIR, "vendor/minitrace/minitrace.h"),
		path.join(ROOT_DIR, "vendor/minitrace/minitrace.c"),
	}
	
	defines "MTR_ENABLED"