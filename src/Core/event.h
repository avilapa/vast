#pragma once

#include "Core/log.h"
#include "Core/types.h"

#include <functional>
#include <variant>

#define VAST_EVENT_HANDLER(func)						[this](const vast::EventData& data) { (void)data; func(); }
#define VAST_EVENT_HANDLER_DATA(func)					[this](const vast::EventData& data) { func(data); }

#define VAST_SUBSCRIBE_TO_EVENT(eventType, callback)	::vast::EventSystem::SubscribeToEvent<eventType>(callback)
#define VAST_FIRE_EVENT_DATA(eventType, eventData)		::vast::EventSystem::FireEvent<eventType>(eventData)
#define VAST_FIRE_EVENT(eventType)						::vast::EventSystem::FireEvent<eventType>()

namespace vast
{
	// Add event types as needed, here as well as a matching class using CREATE_EVENT_TYPE()
	enum class EventType
	{
		NONE = 0,
		WindowClose, WindowResize,
		COUNT
	};

	class IEvent
	{
	public:
		virtual ~IEvent() = default;
		virtual EventType GetType() const = 0;
	};

#define CREATE_EVENT_TYPE(className)											\
	class className##Event : public IEvent										\
	{																			\
	public:																		\
		className##Event() = default;											\
		static EventType GetStaticType() { return EventType::className; }		\
		virtual EventType GetType() const override { return GetStaticType(); }	\
		static std::string GetName() { return STR(className##Event); }			\
	};

	CREATE_EVENT_TYPE(WindowClose);
	CREATE_EVENT_TYPE(WindowResize);

// Add data types as needed by the callback functions.
#define EVENT_DATA_TYPES	\
	uint2

	// TODO: std::variant seems to throw a lot of warnings, annoying.
	// TODO: Could make a wrapper for std::variant to make the intended usage more obvious.
	using EventData = std::variant<EVENT_DATA_TYPES>;

	class EventSystem
	{
	public:
		using EventCallback = std::function<void(const EventData&)>;

		template<typename T>
		static void SubscribeToEvent(EventCallback&& func)
		{
			uint32 eventIdx = static_cast<uint32>(T::GetStaticType());
			VAST_TRACE("[events] {} registered new subscriber ({})", T::GetName(), GetSubscriberCount(eventIdx));
			s_EventsSubscribers[eventIdx].push_back(std::forward<EventCallback>(func));

		}

		template<typename T>
		static void FireEvent(const EventData& data = 0)
		{
			uint32 eventIdx = static_cast<uint32>(T::GetStaticType());
			VAST_TRACE("[events] {} fired. Executing {} subscriber callbacks...", T::GetName(), GetSubscriberCount(eventIdx));
			for (const auto& sub : s_EventsSubscribers[eventIdx])
			{
				sub(data);
			}
		}

		static uint32 GetSubscriberCount(uint32 eventIdx)
		{
			return s_EventsSubscribers[eventIdx].size();
		}

	private:
		static std::array<std::vector<EventSystem::EventCallback>, static_cast<uint32>(EventType::COUNT)> s_EventsSubscribers;
	};
}
