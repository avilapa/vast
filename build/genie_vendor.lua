ROOT_DIR = path.getabsolute("../")

project "imgui"
	kind "StaticLib"
	language "C++"
	
	files
	{
		path.join(ROOT_DIR, "vendor/imgui/im*.h"),
		path.join(ROOT_DIR, "vendor/imgui/im*.cpp"),
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
	
project "dx12"
	kind "StaticLib"
	language "C++"
	
	files
	{
		path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/include/**.h"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/include/**.idl"),--?
		
		path.join(ROOT_DIR, "vendor/dx12/D3D12MemoryAllocator/include/D3D12MemAlloc.h"),
		path.join(ROOT_DIR, "vendor/dx12/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"),

		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/*.h"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/*.cpp"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/*.inl"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/Shaders/Compiled/*.inc"),
		
		path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/inc/*.h"),
	}
	
	includedirs
	{
		path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/include/"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/include/d3dx12"),
		path.join(ROOT_DIR, "vendor/dx12/D3D12MemoryAllocator/include/"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/Shaders/Compiled/"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/inc/"),
	}
	
	libdirs
	{
		path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/lib/x64/"),
	}
	
	links
	{
		"d3d12", "dxgi", "dxguid", "dxcompiler",
	}