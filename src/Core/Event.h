#pragma once

#include "Core/Core.h"
#include "Core/Types.h"

#include <functional>

// ==================================== EVENT SYSTEM USAGE ========================================
//
// This is a blocking event system. New event types can be added to event_types.h. Event callbacks
// are bound by subscribing to an event type using VAST_SUBSCRIBE_TO_EVENT or if the callback needs
// data to resolve itself, using VAST_SUBSCRIBE_TO_EVENT_DATA instead:
//
//		VAST_SUBSCRIBE_TO_EVENT(MyCoolEvent, MyClass::OnCoolEvent);
//		VAST_SUBSCRIBE_TO_EVENT_DATA(MyCoolEvent, MyClass::OnCoolEvent);
//
// The callback function must return void and be declared with either no parameters or only one 
// parameter, that being a reference to the event type the function is subscribed to:
//
//		void MyClass::OnCoolEvent();
//		void MyClass::OnCoolEvent(MyCoolEvent& event);
//
// Any number of functions can be subscribed to an event. When the event is fired, all callbacks 
// are executed. Events are fired using VAST_FIRE_EVENT, with an optional second parameter, that
// being the event type, which contains the data necessary for the callback functions to resolve
// the event:
//
//		MyCoolEvent event(...);
//		VAST_FIRE_EVENT(MyCoolEvent, event);
//
// ================================================================================================

#define __VAST_EVENT_CALLBACK_DATA(eventType, callback)		[this](vast::IEvent& data) { callback(dynamic_cast<eventType&>(data)); }
#define __VAST_EVENT_CALLBACK(eventType, callback)			[this](vast::IEvent& data) { (void)data; callback(); }
// TODO: I'd like to have only one VAST_SUBSCRIBE_TO_EVENT macro that can identify on compile time if the callback's signature has any parameters.
// If you are crashing here, it is likely that you are passing an invalid type to this macro (needs to inherit from IEvent).
#define VAST_SUBSCRIBE_TO_EVENT_DATA(eventType, callback)	::vast::EventSystem::SubscribeToEvent<eventType>(__VAST_EVENT_CALLBACK_DATA(eventType, callback))
#define VAST_SUBSCRIBE_TO_EVENT(eventType, callback)		::vast::EventSystem::SubscribeToEvent<eventType>(__VAST_EVENT_CALLBACK(eventType, callback))

#define __VAST_FIRE_EVENT_DATA(eventType, eventData)		::vast::EventSystem::FireEvent<eventType>(eventData)
#define __VAST_FIRE_EVENT(eventType)						::vast::EventSystem::FireEvent<eventType>()

#define VAST_FIRE_EVENT(...) EXP(SELECT_MACRO(__VA_ARGS__, __VAST_FIRE_EVENT_DATA, __VAST_FIRE_EVENT)(__VA_ARGS__))

namespace vast
{
	enum class EventType;

	class IEvent
	{
	public:
		IEvent() = default;
		virtual ~IEvent() = default;
		virtual EventType GetType() const { return static_cast<EventType>(0); };
	};

#define EVENT_CLASS_DECL_TYPE(type)											\
	static EventType GetStaticType() { return EventType::type; }			\
	virtual EventType GetType() const override { return GetStaticType(); }	\
	static std::string GetName() { return STR(type##Event); }

	class EventSystem
	{
	public:
		using EventCallback = std::function<void(IEvent&)>;

		template<typename T>
		static void SubscribeToEvent(EventCallback&& func)
		{
			VAST_PROFILE_FUNCTION();
			uint32 eventIdx = static_cast<uint32>(T::GetStaticType());
			VAST_TRACE("[events] {} registered new subscriber ({})", T::GetName(), (GetSubscriberCount(eventIdx) + 1));
			SubscribeToEvent(eventIdx, std::move(func));
		}

		template<typename T>
		static void FireEvent()
		{
			T data = T();
			FireEvent<T>(data);
		}

		template<typename T>
		static void FireEvent(IEvent& data)
		{
			VAST_PROFILE_FUNCTION();
			uint32 eventIdx = static_cast<uint32>(T::GetStaticType());
			VAST_TRACE("[events] {} fired. Executing {} subscriber callbacks...", T::GetName(), GetSubscriberCount(eventIdx));
			FireEvent(eventIdx, data);
		}

		static uint32 GetSubscriberCount(uint32 eventIdx);

	private:
		static void SubscribeToEvent(uint32 eventIdx, EventCallback&& func);
		static void FireEvent(uint32 eventIdx, IEvent& data);
	};
}