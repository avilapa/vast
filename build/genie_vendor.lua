
project "imgui"
	kind "StaticLib"
	language "C++"
	
	includedirs
	{
		path.join(ROOT_DIR, "src/"),
		path.join(ROOT_DIR, "vendor/imgui"), -- Only necessary while we use Imgui stock implementation on win32
	}
	
	files
	{
		path.join(ROOT_DIR, "vendor/imgui/im*.h"),
		path.join(ROOT_DIR, "vendor/imgui/im*.cpp"),
		path.join(ROOT_DIR, "vendor/imgui/backends/imgui_impl_win32.h"),
		path.join(ROOT_DIR, "vendor/imgui/backends/imgui_impl_win32.cpp"),
	}

	defines { 'IMGUI_USER_CONFIG="Rendering/Imgui.h"', "IMGUI_LIB"  }
	
spdlog =
{
	files 		= path.join(ROOT_DIR, "vendor/spdlog/include/**.h"),
	includedirs = path.join(ROOT_DIR, "vendor/spdlog/include"),
}	

hlslpp =
{
	files 		= path.join(ROOT_DIR, "vendor/hlslpp/include/**.h"),
	includedirs = path.join(ROOT_DIR, "vendor/hlslpp/include"),
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
	
DirectXShaderCompiler =
{
	files 		= path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/inc/*.h"),
	includedirs = path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/inc/"),
	libdirs 	= path.join(ROOT_DIR, "vendor/dx12/DirectXShaderCompiler/lib/x64/"),
	links		= "dxcompiler"
}

DirectXAgilitySDK =
{
	files 		= path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/include/*"),
	includedirs = path.join(ROOT_DIR, "vendor/dx12/DirectXAgilitySDK/include/"),
	links		= { "d3d12", "dxgi", "dxguid" } -- dxguid?
}
	
project "D3D12MemoryAllocator"
	kind "StaticLib"
	language "C++"
	
	files
	{
		path.join(ROOT_DIR, "vendor/dx12/D3D12MemoryAllocator/include/D3D12MemAlloc.h"),
		path.join(ROOT_DIR, "vendor/dx12/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"),
	}
	
	includedirs
	{
		path.join(ROOT_DIR, "vendor/dx12/D3D12MemoryAllocator/include/"),
	}
	
project "DirectXTex"
	kind "StaticLib"
	language "C++"
	
	AddLibrary(DirectXAgilitySDK)
	
	files
	{
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/*.h"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/*.cpp"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/*.inl"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/Shaders/Compiled/*.inc"),
	}
	
	includedirs
	{
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/"),
		path.join(ROOT_DIR, "vendor/dx12/DirectXTex/DirectXTex/Shaders/Compiled/"),
	}
	
function AddDX12Libraries()
	AddLibrary(DirectXAgilitySDK)
	AddLibrary(DirectXShaderCompiler)
	links { "D3D12MemoryAllocator", "DirectXTex" }
end