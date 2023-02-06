ROOT_DIR = path.getabsolute("../")

solution "vast"
	location (path.join(ROOT_DIR, "build/vs"))
	
	configurations { "Debug", "Release" }
	platforms { "x64" }
		
	language "C++"
	flags { "CppLatest" }
	
	-- TODO: Cannot figure out how to make folders for each project inside bin, .lib files from StaticLib projects stop generating for some reason.
	-- TODO: Add platform specific folders
	configuration "Debug"
		defines 	{ "VAST_DEBUG" }
		flags 		{ "Symbols", "ExtraWarnings" }
		targetdir 	(path.join(ROOT_DIR, "build/bin/Debug"))
		debugdir	(path.join(ROOT_DIR, "build/bin/Debug"))
        objdir 		(path.join(ROOT_DIR, "build/obj/Debug"))
		
	configuration "Release"
		defines 	{ "VAST_RELEASE" }
		flags 		{ "Optimize" }
		targetdir 	(path.join(ROOT_DIR, "build/bin/Release"))
		objdir 		(path.join(ROOT_DIR, "build/obj/Release"))
		
	configuration "vs2019"
	windowstargetplatformversion "10.0"

	startproject "dev"

dofile("genie_vast.lua")
dofile("genie_vendor.lua")
dofile("genie_dev.lua")
