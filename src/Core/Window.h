#pragma once

#include "Core/Core.h"

namespace vast
{

	class Window
	{
	public:
		static Ptr<Window> Create();
		virtual ~Window() = default;

		virtual void Update() = 0;

		// Size here refers to the client area rather than the total dimensions of the window.
		virtual void SetSize(uint2 newSize) = 0;
		virtual uint2 GetSize() const = 0;
		virtual float GetAspectRatio() const = 0;

		virtual void SetName(const std::string& name) = 0;
		virtual std::string GetName() const = 0;
	};
}