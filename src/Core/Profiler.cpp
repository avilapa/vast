#include "vastpch.h"
#include "Core/Profiler.h"
#include "Core/Types.h"
#include "Core/Timer.h"
#include "Graphics/GraphicsContext.h"

#include "minitrace/minitrace.h"

namespace vast
{

	static const uint32 kInvalidProfilingEntryIdx = UINT32_MAX;
	static const uint32 kMaxProfilingEntries = gfx::NUM_TIMESTAMP_QUERIES / 2;

	struct ProfilingEntry
	{
		std::string name;

		enum class State
		{
			IDLE = 0,
			ACTIVE,
			FINISHED
		} state = State::IDLE;

		static const uint32 kHistorySize = 16;
		Array<double, kHistorySize> deltasHistory;
		uint32 currDelta = 0;
	};

	static Array<ProfilingEntry, kMaxProfilingEntries> s_ProfilingEntries;
	static uint32 s_ProfilingEntryCount = 0;
	static uint32 s_ActiveProfilingEntry = kInvalidProfilingEntryIdx;

	void Profiler::Init(const char* fileName)
	{
		mtr_init(fileName);
	}

	void Profiler::Stop()
	{
		mtr_flush(); 
		mtr_shutdown();
	}

	void Profiler::EndFrame(gfx::GraphicsContext& ctx)
	{
		VAST_ASSERT(s_ActiveProfilingEntry == kInvalidProfilingEntryIdx);

		if (s_ProfilingEntryCount == 0)
			return;

		const uint64* queryData = ctx.ResolveTimestamps(s_ProfilingEntryCount * 2);

		const double gpuFrequency = static_cast<double>(ctx.GetTimestampFrequency());

		for (uint32 i = 0; i < s_ProfilingEntryCount; ++i)
		{
			double time = 0;

			uint64 startTime = queryData[i * 2];
			uint64 endTime = queryData[i * 2 + 1];

			if (endTime > startTime)
			{
				uint64 delta = endTime - startTime;
				time = (delta / gpuFrequency) * 1000.0;
			}

			ProfilingEntry& e = s_ProfilingEntries[i];
			e.deltasHistory[e.currDelta] = time;
			e.currDelta = (e.currDelta + 1) & ProfilingEntry::kHistorySize;
			e.state = ProfilingEntry::State::IDLE;

			// TODO: We could compute min, max, avg, etc from the history
			VAST_INFO("'{}' Time: {} ms", e.name, time);
		}
	}

	void Profiler::BeginCpuTiming(const char* name)
	{

	}

	void Profiler::EndCpuTiming(const char* name)
	{

	}

	void Profiler::BeginGpuTiming(const char* name, gfx::GraphicsContext& ctx)
	{
		// TODO: Currently stacking timings is not supported.
		VAST_ASSERT(s_ActiveProfilingEntry == kInvalidProfilingEntryIdx);

		// Check if profile already exists
		for (uint32 i = 0; i < s_ProfilingEntryCount; ++i)
		{
			if (s_ProfilingEntries[i].name == name)
			{
				s_ActiveProfilingEntry = i;
				break;
			}
		}

		if (s_ActiveProfilingEntry == kInvalidProfilingEntryIdx)
		{
			// Add new profile
			VAST_ASSERTF(s_ProfilingEntryCount < kMaxProfilingEntries, "Exceeded max number of unique profiling markers ({}).", kMaxProfilingEntries);
			s_ActiveProfilingEntry = s_ProfilingEntryCount++;
			s_ProfilingEntries[s_ActiveProfilingEntry].name = name;
		}

		VAST_ASSERTF(s_ProfilingEntries[s_ActiveProfilingEntry].state == ProfilingEntry::State::IDLE, "Profiling marker '{}' already pushed this frame.", name);
		s_ProfilingEntries[s_ActiveProfilingEntry].state = ProfilingEntry::State::ACTIVE;

		ctx.InsertTimestamp(s_ActiveProfilingEntry * 2);
	}

	void Profiler::EndGpuTiming(const char* name, gfx::GraphicsContext& ctx)
	{
		// TODO: Currently stacking timings is not supported.
		VAST_ASSERT(s_ActiveProfilingEntry != kInvalidProfilingEntryIdx);

		VAST_ASSERT(s_ProfilingEntries[s_ActiveProfilingEntry].state == ProfilingEntry::State::ACTIVE);

		ctx.InsertTimestamp(s_ActiveProfilingEntry * 2 + 1);

		s_ProfilingEntries[s_ActiveProfilingEntry].state = ProfilingEntry::State::FINISHED;
		s_ActiveProfilingEntry = kInvalidProfilingEntryIdx;
	}

	void Profiler::BeginTrace(const char* category, const char* name)
	{
		MTR_BEGIN(category, name);
	}

	void Profiler::EndTrace(const char* category, const char* name)
	{
		MTR_END(category, name);
	}

}
