#include "vastpch.h"
#include "Core/event.h"

namespace vast
{

	std::array<std::vector<EventSystem::EventCallback>, static_cast<uint32>(EventType::COUNT)> EventSystem::s_EventsSubscribers;

}