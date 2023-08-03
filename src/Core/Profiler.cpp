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

	enum class ProfilingEntryState
	{
		IDLE = 0,	// Waiting to start
		ACTIVE,		// Started timing
		FINISHED	// Finished timing but not resolved yet
	};

	struct ProfilingEntry
	{
		std::string name;
		ProfilingEntryState state = ProfilingEntryState::IDLE;

		static const uint32 kHistorySize = 64;
		Array<double, kHistorySize> deltasHistory;
		uint32 currDelta = 0;

		gfx::GraphicsContext* ctx = nullptr;
		int64 tStart;
		int64 tEnd;
	};

	static Array<ProfilingEntry, kMaxProfilingEntries> s_ProfilingEntries;
	static uint32 s_TotalEntryCount = 0;

	static Timer s_Timer;

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
		if (s_TotalEntryCount == 0)
			return;

		const uint64* queryData = ctx.ResolveTimestamps(s_TotalEntryCount * 2);
		const double gpuFrequency = static_cast<double>(ctx.GetTimestampFrequency());

		for (uint32 i = 0; i < s_TotalEntryCount; ++i)
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
			e.state = ProfilingEntryState::IDLE;
		}
	}

	void Profiler::BeginTiming(const char* name, gfx::GraphicsContext* ctx /* = nullptr */)
	{
		// Check if profile name already exists
		uint32 idx = kInvalidProfilingEntryIdx;
		for (uint32 i = 0; i < s_TotalEntryCount; ++i)
		{
			if (s_ProfilingEntries[i].name == name)
			{
				idx = i;
				break;
			}
		}

		if (idx == kInvalidProfilingEntryIdx)
		{
			// Add new profile
			VAST_ASSERTF(s_TotalEntryCount < kMaxProfilingEntries, "Exceeded max number of unique profiling markers ({}).", kMaxProfilingEntries);
			idx = s_TotalEntryCount++;
			s_ProfilingEntries[idx].name = name;
			if (ctx) 
			s_ProfilingEntries[idx].ctx = ctx;
		}

		VAST_ASSERTF(ctx == s_ProfilingEntries[idx].ctx);

		if (s_ProfilingEntries[idx].ctx)
		{
			// GPU Timing
			s_ProfilingEntries[idx].ctx->InsertTimestamp(idx * 2);
		}
		else
		{
			// CPU Timing
			s_Timer.Update();
			s_ProfilingEntries[idx].tStart = s_Timer.GetElapsedMicroseconds<int64>();
		}

		VAST_ASSERTF(s_ProfilingEntries[idx].state == ProfilingEntryState::IDLE, "Profiling marker '{}' already pushed this frame.", name);
		s_ProfilingEntries[idx].state = ProfilingEntryState::ACTIVE;
	}

	void Profiler::EndTiming()
	{
		// Find last active profile
		uint32 idx = kInvalidProfilingEntryIdx;
		for (uint32 i = s_TotalEntryCount; i >= 0; --i)
		{
			if (s_ProfilingEntries[i].state == ProfilingEntryState::ACTIVE)
			{
				idx = i;
				break;
			}
		}
		
		VAST_ASSERT(idx != kInvalidProfilingEntryIdx);
		VAST_ASSERT(s_ProfilingEntries[idx].state == ProfilingEntryState::ACTIVE);

		if (s_ProfilingEntries[idx].ctx)
		{
			// GPU Timing
			s_ProfilingEntries[idx].ctx->InsertTimestamp(idx * 2 + 1);
		}
		else
		{
			// CPU Timing
			s_Timer.Update();
			s_ProfilingEntries[idx].tEnd = s_Timer.GetElapsedMicroseconds<int64>();
		}

		s_ProfilingEntries[idx].state = ProfilingEntryState::FINISHED;
		idx = kInvalidProfilingEntryIdx;
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

			for (uint32 i = 0; i < s_TotalEntryCount; ++i)
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
