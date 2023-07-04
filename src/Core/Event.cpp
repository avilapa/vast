#include "vastpch.h"
#include "Core/Event.h"
#include "Core/EventTypes.h"

#include <map>

namespace vast
{

	static Array<std::map<EventSystem::SubscriberKey, EventSystem::EventHandler>, static_cast<uint32>(EventType::COUNT)> s_EventsSubscribers;

	void EventSystem::SubscribeToEvent(SubscriberKey key, EventHandler&& func)
	{
		s_EventsSubscribers[key.first].insert({ key, func });
	}

	void EventSystem::UnsubscribeFromEevent(SubscriberKey key)
	{
		if (s_EventsSubscribers[key.first].find(key) != s_EventsSubscribers[key.first].end())
		{
			s_EventsSubscribers[key.first].erase(key);
		}
		else
		{
			VAST_ASSERTF(0, "Attempted to unsubscribe unknown id from event.");
		}
	}

	void EventSystem::FireEvent(uint32 eventIdx, IEvent& data)
	{
		VAST_PROFILE_FUNCTION();

		for (const auto& [key, callback] : s_EventsSubscribers[eventIdx])
		{
			callback(data);
		}
	}

	uint32 EventSystem::GetSubscriberCount(uint32 eventIdx)
	{
		return static_cast<uint32>(s_EventsSubscribers[eventIdx].size());
	}
}