
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

function CopyDebugDLLs(projDir)
	postbuildcommands
	{
		'echo Copying DX12 necessary DLLs to bin',
		'xcopy /Y /D "'..path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/bin/x64\\*.dll")..'" "'..path.join(projDir, "build/bin/Debug/")..'"',
		'xcopy /Y /D "'..path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/bin/x64\\*.dll")..'" "'..path.join(projDir, "build/bin/Debug/D3D12/*")..'"',
	}
end

function CopyReleaseDLLs(projDir)
	postbuildcommands
	{
		'echo Copying DX12 necessary DLLs to bin',
		'xcopy /Y /D "'..path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/bin/x64\\*.dll")..'" "'..path.join(projDir, "build/bin/Release/")..'"',
		'xcopy /Y /D "'..path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/bin/x64\\*.dll")..'" "'..path.join(projDir, "build/bin/Release/D3D12/*")..'"',
	}
end

function AddProject(proj)
	local filePath = path.join(ROOT_DIR, "projects/" .. proj .. "/build/genie.lua")
	local file = io.open(filePath)
	if file then
		file:close()
		print("Generating Project: " .. proj)
		dofile(filePath)
	end
end

AddProject("dev")
AddProject("samples")
AddProject("forge")
