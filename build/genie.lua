
ROOT_DIR = path.getabsolute("../")

solution "vast"
	location (path.join(ROOT_DIR, "build/vs"))
	
	configurations { "Debug", "Release" }
	platforms { "x64" }
		
	language "C++"
	flags { "CppLatest" }
	
	-- TODO: Add platform specific folders
	configuration "Debug"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Debug"))
        objdir 		(path.join(ROOT_DIR, "build/obj/Debug"))
		flags 		{ "Symbols", "ExtraWarnings" }
		defines 	{ "VAST_DEBUG" }
		
	configuration "Release"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Release"))
		objdir 		(path.join(ROOT_DIR, "build/obj/Release"))
		flags 		{ "Optimize" }
		defines 	{ "VAST_RELEASE" }
		
	startproject "dev"
	
function AddLibrary(lib)
	if nil ~= lib.files 		then
		files(lib.files)
	end
	if nil ~= lib.includedirs 	then
		includedirs(lib.includedirs)
	end
	if nil ~= lib.libdirs 		then
		libdirs(lib.libdirs)
	end
	if nil ~= lib.links 		then
		links(lib.links)
	end
end

group "vendor"
dofile("genie_vendor.lua")
group "engine"
dofile("genie_vast.lua")
group "projects"
dofile("genie_dev.lua")
dofile("genie_samples.lua")
