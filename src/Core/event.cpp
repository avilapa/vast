#include "vastpch.h"
#include "Core/event.h"

namespace vast
{

// 	void EventSystem::SubscribeToEvent(const uint32 eventIdx, EventSubscriber&& func)
// 	{
// 		s_EventsSubscribers[eventIdx].push_back(std::forward<EventSubscriber>(std::forward<EventSubscriber>(func)));
// 	}
// 
// 	void EventSystem::FireEvent(const uint32 eventIdx, const EventData& data)
// 	{
// 		for (const auto& sub : s_EventsSubscribers[eventIdx])
// 		{
// 			sub(data);
// 		}
// 	}

}