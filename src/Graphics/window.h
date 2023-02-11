#pragma once

#include "Core/core.h"
#include "Core/types.h"

namespace vast
{
	struct WindowParams
	{
		WindowParams() : name("vast Engine"), size(1600, 900) {}

		std::string name;
		uint2 size;
	};

	class Window
	{
	public:
		static std::unique_ptr<Window> Create(const WindowParams& params = WindowParams());
		virtual ~Window() = default;

		virtual void OnUpdate() = 0;
		virtual uint2 GetWindowSize() const = 0;
	};
}