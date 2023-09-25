#pragma once

// Note: Imgui project files define IMGUI_USER_CONFIG to replace imconfig.h with this file.
namespace vast::gfx
{
    template<typename T> class Handle;
    using TextureHandle = Handle<class Texture>;
}
#define ImTextureID vast::gfx::TextureHandle*

#if !defined(IMGUI_LIB)
#include "imgui/imgui.h"
#endif
