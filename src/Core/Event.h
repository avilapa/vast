#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <functional>

// ==================================== EVENT SYSTEM USAGE ========================================
//
// This is a blocking event system. New event types can be added to EventTypes.h. Event callbacks
// are bound by subscribing to an event type using VAST_SUBSCRIBE_TO_EVENT and passing a subscriber
// id string, the event class type and a callback (passed using VAST_EVENT_CALLBACK) as parameters:
//
//		VAST_SUBSCRIBE_TO_EVENT("myclass", MyCoolEvent, VAST_EVENT_CALLBACK(MyClass::OnCoolEvent));
//		VAST_SUBSCRIBE_TO_EVENT("myclass", MyCoolEvent, VAST_EVENT_CALLBACK(MyClass::OnCoolEvent, MyCoolEvent));
//
// The callback function must return void and be declared with either no parameters or only one 
// parameter, that being a const reference to the event type the function is subscribed to:
//
//		void MyClass::OnCoolEvent();
//		void MyClass::OnCoolEvent(MyCoolEvent& event);
// 
// When subscribing object is destroyed or event listener is no longer needed the subscriber needs
// to be deregistered using VAST_UNSUBSCRIBE_FROM_EVENT, passing ass parameters the subscriber id
// and the event type.
// 
//		VAST_UNSUBSCRIBE_FROM_EVENT("myclass", MyCoolEvent);
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

// TODO: Currently this event system is not thread safe.

namespace vast
{

#define __VAST_EVENT_HANDLER_CB_DATA(fn, eventType) [this](const IEvent& data) { fn(static_cast<const eventType&>(data)); }
#define __VAST_EVENT_HANDLER_CB(fn) [this](const IEvent&) { fn(); }

#define __VAST_EVENT_HANDLER_CB_STATIC_DATA(fn, eventType) [](const IEvent& data) { fn(static_cast<const eventType&>(data)); }
#define __VAST_EVENT_HANDLER_CB_STATIC(fn) [](const IEvent&) { fn(); }

#define VAST_EVENT_HANDLER_CB(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_EVENT_HANDLER_CB_DATA, __VAST_EVENT_HANDLER_CB)(__VA_ARGS__))
#define VAST_EVENT_HANDLER_CB_STATIC(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_EVENT_HANDLER_CB_STATIC_DATA, __VAST_EVENT_HANDLER_CB_STATIC)(__VA_ARGS__))
#define VAST_EVENT_HANDLER_EXP(expression) [this](const IEvent&) { expression; }
#define VAST_EVENT_HANDLER_EXP_STATIC(expression) [](const IEvent&) { expression; }

#define VAST_SUBSCRIBE_TO_EVENT(subId, eventType, fn) EventSystem::SubscribeToEvent<eventType>(subId, fn)
#define VAST_UNSUBSCRIBE_FROM_EVENT(subId, eventType) EventSystem::UnsubscribeFromEvent<eventType>(subId)

#define __VAST_FIRE_EVENT_DATA(eventType, data)	EventSystem::FireEvent<eventType>(data)
#define __VAST_FIRE_EVENT(eventType) EventSystem::FireEvent<eventType>()

#define VAST_FIRE_EVENT(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_FIRE_EVENT_DATA, __VAST_FIRE_EVENT)(__VA_ARGS__))

	enum class EventType;

	class IEvent
	{
	public:
		virtual ~IEvent() = default;
	};

#define EVENT_CLASS_DECL_STATIC_TYPE(type)							\
	static const EventType GetType() { return EventType::type; }	\
	static const char* GetName() { return STR(type##Event); }

	class EventSystem
	{
	public:
		using EventHandler = std::function<void(const IEvent&)>;
		using SubscriberKey = std::pair<uint32, std::string>;

		template<typename T>
		static void SubscribeToEvent(std::string subscriberIdx, EventHandler&& func)
		{
			const uint32 eventIdx = static_cast<uint32>(T::GetType());
			const SubscriberKey key = std::make_pair(eventIdx, subscriberIdx);

			SubscribeToEvent(key, std::move(func));
			VAST_TRACE("[events] {} registered new subscriber ({}).", T::GetName(), (GetSubscriberCount(eventIdx)));
		}

		template<typename T>
		static void UnsubscribeFromEvent(std::string subscriberIdx)
		{
			const uint32 eventIdx = static_cast<uint32>(T::GetType());
			const SubscriberKey key = std::make_pair(eventIdx, subscriberIdx);

			UnsubscribeFromEevent(key);
			VAST_TRACE("[events] {} deregistered a subscriber ({}).", T::GetName(), (GetSubscriberCount(eventIdx)));
		}

		template<typename T>
		static void FireEvent(IEvent& data)
		{
			const uint32 eventIdx = static_cast<uint32>(T::GetType());
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
		static void SubscribeToEvent(SubscriberKey key, EventHandler&& func);
		static void UnsubscribeFromEevent(SubscriberKey key);
		static void FireEvent(uint32 eventIdx, IEvent& data);
	};
}