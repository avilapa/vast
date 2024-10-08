#pragma once

#include "Core/Core.h"

struct ShaderDebug_PerFrame;

namespace vast
{

	class ShaderDebug
	{
	public:
		static void Reset(ShaderDebug_PerFrame& data);
		static void OnGUI(ShaderDebug_PerFrame& data);
	};

}
