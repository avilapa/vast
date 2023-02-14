#include "vastpch.h"
#include "Core/event.h"
#include "Core/event_types.h"

namespace vast
{

	static Array<Vector<EventSystem::EventCallback>, static_cast<uint32>(EventType::COUNT)> s_EventsSubscribers;

	void EventSystem::SubscribeToEvent(uint32 eventIdx, EventCallback&& func)
	{
		s_EventsSubscribers[eventIdx].push_back(std::forward<EventCallback>(func));
	}

	void EventSystem::FireEvent(uint32 eventIdx, IEvent& data)
	{
		for (const auto& sub : s_EventsSubscribers[eventIdx])
		{
			sub(data);
		}
	}

	uint32 EventSystem::GetSubscriberCount(uint32 eventIdx)
	{
		return static_cast<uint32>(s_EventsSubscribers[eventIdx].size());
	}
}