
PROJ_DIR = path.getabsolute("../projects/samples")

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
		targetdir 	(path.join(ROOT_DIR, "build/bin/Debug/samples"))
        objdir 		(path.join(ROOT_DIR, "build/obj/Debug/samples"))
		postbuildcommands
		{
			'echo Copying necessary DLLs to bin',
			'copy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/bin/x64\\*.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Debug/samples/")..'" ',
			'xcopy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/bin/x64\\*.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Debug/samples/D3D12/*")..'" /y',
		}
		
	configuration "Release"
		targetdir 	(path.join(ROOT_DIR, "build/bin/Release/samples"))
		objdir 		(path.join(ROOT_DIR, "build/obj/Release/samples"))
		postbuildcommands
		{
			'echo Copying necessary DLLs to bin',
			'copy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/bin/x64\\*.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Release/samples/")..'" ',
			'xcopy "'..path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/bin/x64\\D3D12Core.dll")..'" "'..path.join(ROOT_DIR, "build/bin/Release/samples/D3D12/*")..'" /y',
		}
	