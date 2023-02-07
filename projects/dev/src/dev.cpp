#include "dev.h"

VAST_DEFINE_APP_MAIN(Dev)

using namespace vast;

void Dev::OnWindowClose()
{
	VAST_INFO("Window Closed");
}

void Dev::OnWindowResize(const EventData& data)
{
	uint2 eventData = std::get<uint2>(data);
	VAST_INFO("Window Resized to: {},{}", static_cast<int>(eventData.x), static_cast<int>(eventData.y));
}

void Dev::Run()
{
	Log::Init();
	VAST_TRACE("Tracing somethihng......");
	VAST_INFO("Thats nice info");
	VAST_WARNING("Initialized Log {:03.2f}", 1.23456);
	VAST_ERROR("NOOOOOOOOOOOOOOOOOO");
	VAST_CRITICAL("RIP");

	EventData asd = uint2(5, 13);
	VAST_SUBSCRIBE_TO_EVENT(WindowCloseEvent, VAST_EVENT_HANDLER_DATA(Dev::OnWindowResize));
	VAST_FIRE_EVENT_DATA(WindowCloseEvent, asd);

	VAST_SUBSCRIBE_TO_EVENT(WindowResizeEvent, VAST_EVENT_HANDLER(Dev::OnWindowClose));
	VAST_FIRE_EVENT(WindowResizeEvent);

	VAST_ASSERTF(0, "Big assert coming {} you.", 4);
}