#pragma once

#include "Core/Event.h"

// ================================== ADDING NEW EVENT TYPES ======================================
//
// In order to add a new event type, just add a new entry in the EventType enum, and add a matching
// class. The class must inherit from IEvent and declare EVENT_CLASS_DECL_TYPE inside the body of
// the class, with the EventType as parameter. Feel free to add any data or functions necessary to 
// resolve the events in the class.
//
// ================================================================================================

namespace vast
{
	enum class EventType
	{
		NONE = 0,
		WindowClose, WindowResize,
		DebugAction,
		ReloadShaders,
		COUNT
	};

	class WindowCloseEvent final : public IEvent
	{
	public:
		WindowCloseEvent() = default;
		EVENT_CLASS_DECL_STATIC_TYPE(WindowClose);
	};

	class WindowResizeEvent final : public IEvent
	{
	public:
		EVENT_CLASS_DECL_STATIC_TYPE(WindowResize);
		WindowResizeEvent(uint2 windowSize) : m_WindowSize(windowSize) {}
		uint2 m_WindowSize;
	};

	class DebugActionEvent final : public IEvent
	{
	public:
		DebugActionEvent() = default;
		EVENT_CLASS_DECL_STATIC_TYPE(DebugAction);
	};

	class ReloadShadersEvent final : public IEvent
	{
	public:
		ReloadShadersEvent() = default;
		EVENT_CLASS_DECL_STATIC_TYPE(ReloadShaders);
		ReloadShadersEvent(gfx::GraphicsContext& ctx_) : ctx(ctx_) {}
		gfx::GraphicsContext& ctx;
	};
}
