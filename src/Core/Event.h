#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <functional>

// ==================================== EVENT SYSTEM USAGE ========================================
//
// This is a blocking event system. New event types can be added to EventTypes.h. Event callbacks
// are bound by subscribing to an event type using VAST_SUBSCRIBE_TO_EVENT and passing the event
// class type and a callback (passed using VAST_EVENT_CALLBACK) as parameters:
//
//		VAST_SUBSCRIBE_TO_EVENT(MyCoolEvent, VAST_EVENT_CALLBACK(MyClass::OnCoolEvent));
//		VAST_SUBSCRIBE_TO_EVENT(MyCoolEvent, VAST_EVENT_CALLBACK(MyClass::OnCoolEvent, MyCoolEvent));
//
// The callback function must return void and be declared with either no parameters or only one 
// parameter, that being a const reference to the event type the function is subscribed to:
//
//		void MyClass::OnCoolEvent();
//		void MyClass::OnCoolEvent(MyCoolEvent& event);
//
// Any number of functions can be subscribed to an event. When the event is fired, all callbacks 
// are executed. Events are fired using VAST_FIRE_EVENT, with an optional second parameter, that
// being the event type, which contains the data necessary for the callback functions to resolve
// the event:
//
//		MyCoolEvent event; // Initialize event with data
//		VAST_FIRE_EVENT(MyCoolEvent, event);
//
// ================================================================================================

// TODO: Callback lifetimes are not managed. If an event is fired after a subscriber has been 
// destroyed we will crash. We should be asserting and provide a way for subscribers to unsubscribe.

// TODO: Currently this event system is not thread safe.

namespace vast
{

#define __VAST_EVENT_CALLBACK_DATA(fn, eventType) [this](const IEvent& data) { fn(static_cast<const eventType&>(data)); }
#define __VAST_EVENT_CALLBACK(fn) [this](const IEvent&) { fn(); }

#define __VAST_EVENT_CALLBACK_STATIC_DATA(fn, eventType) [](const IEvent& data) { fn(static_cast<const eventType&>(data)); }
#define __VAST_EVENT_CALLBACK_STATIC(fn) [](const IEvent&) { fn(); }

#define __VAST_FIRE_EVENT_DATA(eventType, data)	EventSystem::FireEvent<eventType>(data)
#define __VAST_FIRE_EVENT(eventType) EventSystem::FireEvent<eventType>()


#define VAST_EVENT_CALLBACK(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_EVENT_CALLBACK_DATA, __VAST_EVENT_CALLBACK)(__VA_ARGS__))
#define VAST_EVENT_CALLBACK_STATIC(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_EVENT_CALLBACK_STATIC_DATA, __VAST_EVENT_CALLBACK_STATIC)(__VA_ARGS__))

#define VAST_SUBSCRIBE_TO_EVENT(eventType, fn) EventSystem::SubscribeToEvent<eventType>(fn)

#define VAST_FIRE_EVENT(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_FIRE_EVENT_DATA, __VAST_FIRE_EVENT)(__VA_ARGS__))

	enum class EventType;

	class IEvent
	{
	public:
		virtual ~IEvent() = default;
	};

#define EVENT_CLASS_DECL_STATIC_TYPE(type)								\
	static constexpr EventType GetType() { return EventType::type; }	\
	static constexpr char* GetName() { return STR(type##Event); }

	class EventSystem
	{
	public:
		using EventCallback = std::function<void(const IEvent&)>;

		template<typename T>
		static void SubscribeToEvent(EventCallback&& func)
		{
			uint32 eventIdx = static_cast<uint32>(T::GetType());
			VAST_TRACE("[events] {} registered new subscriber ({}).", T::GetName(), (GetSubscriberCount(eventIdx) + 1));
			SubscribeToEvent(eventIdx, std::move(func));
		}

		template<typename T>
		static void FireEvent(IEvent& data)
		{
			uint32 eventIdx = static_cast<uint32>(T::GetType());
			VAST_TRACE("[events] {} fired. Executing {} subscriber callbacks...", T::GetName(), GetSubscriberCount(eventIdx));
			FireEvent(eventIdx, data);
		}

		template<typename T>
		static void FireEvent()
		{
			T data = T();
			FireEvent<T>(data);
		}

		static uint32 GetSubscriberCount(uint32 eventIdx);

	private:
		static void SubscribeToEvent(uint32 eventIdx, EventCallback&& func);
		static void FireEvent(uint32 eventIdx, IEvent& data);
	};
}