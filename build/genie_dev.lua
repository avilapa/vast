
PROJ_DIR = path.getabsolute("../projects/dev")

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
		path.join(ROOT_DIR, "shaders"),
		path.join(ROOT_DIR, "vendor"),
	}
	
	links {	"vast" }
	
	configuration "Debug"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Debug/dev"))
        objdir 		(path.join(ROOT_DIR, "build/obj/Debug/dev"))
		postbuildcommands
		{
			'echo Copying necessary DLLs to bin',
			'copy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/bin/x64\\*.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Debug/dev/")..'" ',
			'xcopy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/bin/x64\\*.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Debug/dev/D3D12/*")..'" /y',
		}
		
	configuration "Release"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Release/dev"))
		objdir 		(path.join(ROOT_DIR, "build/obj/Release/dev"))
		postbuildcommands
		{
			'echo Copying necessary DLLs to bin',
			'copy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/bin/x64\\*.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Release/dev/")..'" ',
			'xcopy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/bin/x64\\D3D12Core.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Release/dev/D3D12/*")..'" /y',
		}
	