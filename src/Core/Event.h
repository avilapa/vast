#pragma once

#include "Core/Defines.h"
#include "Core/Log.h"

// ==================================== EVENT SYSTEM USAGE ========================================
//
// This is a blocking event system. New event types can be added to EventTypes.h. Event callbacks
// are bound by subscribing to an event type using Event::Subscribe<T> and passing a unique
// subscriber identifier and an EventHandler&& object. The system provides helper macros for 
// constructing handlers with the right signature in different ways (via member functions, static
// functions, or with customized lambda capture functions, and also expressions). 
//
//     void MyClass::OnCoolEvent();
//     Event::Subscribe<MyCoolEvent>("MyClass", VAST_EVENT_HANDLER(MyClass::OnCoolEvent));
//
// The callback function must return void and it may be declared with either no parameters or only
// one parameter, that being a const reference to the event type the function is subscribed to:
//
//	   static void OnCoolEvent(const MyCoolEvent& event);
//	   auto&& cb = VAST_EVENT_HANDLER_STATIC(OnCoolEvent, MyCoolEvent);
//     Event::Subscribe<MyCoolEvent>("MyClass", cb);
// 
// When the subscriber is destroyed or the event listener is no longer needed, it must unsubscribe 
// from all event types like so:
// 
//	   Event::Unsubscribe<MyCoolEvent>("myclass");
//
// You may only subscribe to an event type once with the same identifier. Any number of subscribes
// can be listening for an event. When an event is fired, all callbacks are executed in the order
// they were registered. Events are fired using Event::Fire<T>(), with an optional parameter that
// contains any data the event callbacks may be expecting. Data must be included in the event type
// class:
//
//		MyCoolEvent event;
//		event.bIsCool = true;
//		Event::Fire<MyCoolEvent>(event);
//
// ================================================================================================

// TODO: Currently this event system is not thread safe.

namespace vast
{

#define VAST_EVENT_HANDLER_EXPR_CAPTURE(expr, ...)			[__VA_ARGS__](const IEvent&) { expr; }
#define VAST_EVENT_HANDLER_EXPR_STATIC(expr)				VAST_EVENT_HANDLER_EXPR_CAPTURE(expr)
#define VAST_EVENT_HANDLER_EXPR(expr)						VAST_EVENT_HANDLER_EXPR_CAPTURE(expr, this)

#define VAST_EVENT_HANDLER_CAPTURE_DATA(fn, eventType, ...)	[__VA_ARGS__](const IEvent& data) { fn(static_cast<const eventType&>(data)); }
#define VAST_EVENT_HANDLER_CAPTURE(fn, ...)					VAST_EVENT_HANDLER_EXPR_CAPTURE(fn(), __VA_ARGS__)

#define __VAST_EVENT_HANDLER_STATIC_DATA(fn, eventType)		VAST_EVENT_HANDLER_CAPTURE_DATA(fn, eventType)
#define __VAST_EVENT_HANDLER_STATIC(fn)						VAST_EVENT_HANDLER_CAPTURE(fn)
#define VAST_EVENT_HANDLER_STATIC(...)						EXP(SELECT_MACRO(__VA_ARGS__, __VAST_EVENT_HANDLER_STATIC_DATA, __VAST_EVENT_HANDLER_STATIC)(__VA_ARGS__))

#define __VAST_EVENT_HANDLER_DATA(fn, eventType)			VAST_EVENT_HANDLER_CAPTURE_DATA(fn, eventType, this)
#define __VAST_EVENT_HANDLER(fn)							VAST_EVENT_HANDLER_CAPTURE(fn, this)
#define VAST_EVENT_HANDLER(...)								EXP(SELECT_MACRO(__VA_ARGS__, __VAST_EVENT_HANDLER_DATA, __VAST_EVENT_HANDLER)(__VA_ARGS__))

	enum class EventType;
	class IEvent;

	class Event
	{
	public:
		using EventHandler = std::function<void(const IEvent&)>;
		using SubscriberKey = std::pair<uint32, std::string>;

		template<typename T>
		static void Subscribe(std::string subscriberIdx, EventHandler&& func)
		{
			const uint32 eventIdx = static_cast<uint32>(T::GetType());
			Subscribe(std::make_pair(eventIdx, subscriberIdx), std::move(func));
			VAST_LOG_TRACE("[events] {} registered new '{}' subscriber (Count: {}).", T::GetName(), subscriberIdx, GetSubscriberCount(eventIdx));
		}

		template<typename T>
		static void Unsubscribe(std::string subscriberIdx)
		{
			const uint32 eventIdx = static_cast<uint32>(T::GetType());
			Unsubscribe(std::make_pair(eventIdx, subscriberIdx));
			VAST_LOG_TRACE("[events] {} deregistered '{}' subscriber (Count: {}).", T::GetName(), subscriberIdx, GetSubscriberCount(eventIdx));
		}
		
		template<typename T>
		static void Fire(IEvent& data)
		{
			const uint32 eventIdx = static_cast<uint32>(T::GetType());
			VAST_LOG_TRACE("[events] {} fired. Executing {} subscriber callbacks...", T::GetName(), GetSubscriberCount(eventIdx));
			Fire(eventIdx, data);
		}

		template<typename T>
		static void Fire()
		{
			T data = T();
			Fire<T>(data);
		}

		static uint32 GetSubscriberCount(uint32 eventIdx);

	private:
		static void Subscribe(SubscriberKey key, EventHandler&& func);
		static void Unsubscribe(SubscriberKey key);
		static void Fire(uint32 eventIdx, IEvent& data);
	};
}