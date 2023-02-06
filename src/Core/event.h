#pragma once

#include "Core/types.h"

#include <variant>
#include <functional>

#define VAST_EVENT_HANDLER(func)						[this](const vast::EventData& data) { func(); }
#define VAST_EVENT_HANDLER_DATA(func)					[this](const vast::EventData& data) { func(data); }

#define VAST_SUBSCRIBE_TO_EVENT(eventType, callback)	::vast::SubscribeToEvent<eventType>(callback)
#define VAST_FIRE_EVENT_DATA(eventType, eventData)		::vast::FireEvent<eventType>(eventData)
#define VAST_FIRE_EVENT(eventType)						::vast::FireEvent<eventType>()

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
		
		bool m_Consumed = false;
	};

#define CREATE_EVENT_TYPE(className)											\
	class className##Event : public IEvent										\
	{																			\
	public:																		\
		className##Event() = default;											\
		static EventType GetStaticType() { return EventType::className; }		\
		virtual EventType GetType() const override { return GetStaticType(); }	\
	};

	CREATE_EVENT_TYPE(WindowClose);
	CREATE_EVENT_TYPE(WindowResize);

// Add data types as needed by the callback functions.
#define EVENT_DATA_TYPES	\
	int8,					\
	int16,					\
	int32,					\
	int64,					\
	int2,					\
	int3,					\
	int4,					\
	uint8,					\
	uint16,					\
	uint32,					\
	uint64,					\
	uint2,					\
	uint3,					\
	uint4,					\
	float,					\
	float2,					\
	float3,					\
	float4					

	using EventData = std::variant<EVENT_DATA_TYPES>;
	using EventSubscriber = std::function<void(const EventData&)>;

	static std::array<std::vector<EventSubscriber>, static_cast<uint32>(EventType::COUNT)> s_EventsSubscribers;

	template<typename T>
	static void SubscribeToEvent(EventSubscriber&& func)
	{
		const uint32 eventId = static_cast<uint32>(T::GetStaticType());
		s_EventsSubscribers[eventId].push_back(std::forward<EventSubscriber>(std::forward<EventSubscriber>(func)));
	}

	template<typename T>
	static void FireEvent(const EventData& data = 0)
	{
		const uint32 eventId = static_cast<uint32>(T::GetStaticType());
		for (const auto& sub : s_EventsSubscribers[eventId])
		{
			sub(data);
		}
	}
}
