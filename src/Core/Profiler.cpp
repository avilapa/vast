#include "vastpch.h"
#include "Core/Profiler.h"
#include "Core/Types.h"
#include "Core/Timer.h"
#include "Graphics/GraphicsContext.h"

#include "minitrace/minitrace.h"
#include "imgui/imgui.h"

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

		static const uint32 kHistorySize = 64;
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
			e.currDelta = (e.currDelta + 1) % ProfilingEntry::kHistorySize;
			e.state = ProfilingEntry::State::IDLE;
		}
	}

	void Profiler::BeginCpuTiming(const char* name)
	{
		(void)name;
	}

	void Profiler::EndCpuTiming(const char* name)
	{
		(void)name;
	}

	void Profiler::BeginGpuTiming(const char* name, gfx::GraphicsContext& ctx)
	{
		// TODO: Currently stacking timings is not supported.
		VAST_ASSERT(s_ActiveProfilingEntry == kInvalidProfilingEntryIdx);

		// Check if profile name already exists
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

	void Profiler::EndGpuTiming(gfx::GraphicsContext& ctx)
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

	//

	void Profiler::OnGUI()
	{
 		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoSavedSettings 
			| ImGuiWindowFlags_NoFocusOnAppearing 
			| ImGuiWindowFlags_NoNav;
		const float pad = 10.0f;
		const ImGuiViewport* v = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(v->WorkPos.x + v->WorkSize.x - pad, v->WorkPos.y + pad), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

		ImGui::SetNextWindowBgAlpha(0.35f);
		if (ImGui::Begin("Profiler", 0, window_flags))
		{
			ImGui::Text("GPU Profiler");
			ImGui::Separator();

			for (uint32 i = 0; i < s_ProfilingEntryCount; ++i)
			{
				ProfilingEntry& e = s_ProfilingEntries[i];

				double tMax = 0.0;
				double tAvg = 0.0;
				uint32 numAvgSamples = 0;
				for (uint32 j = 0; j < ProfilingEntry::kHistorySize; ++j)
				{
					double t = e.deltasHistory[j];

					if (t <= 0.0)
						continue;

					if (t > tMax)
						tMax = t;
					tAvg += t;
					++numAvgSamples;
				}

				if (numAvgSamples > 0)
					tAvg /= double(numAvgSamples);


				ImGui::Text("%s: %.3fms (%.3fms max)", e.name.c_str(), tAvg, tMax);
			}
		}
		ImGui::End();
	}

}
