#include "vastpch.h"
#include "Core/Profiler.h"
#include "Core/Types.h"
#include "Core/Timer.h"
#include "Graphics/GraphicsContext.h"

#include "minitrace/minitrace.h"
#include "imgui/imgui.h"

namespace vast
{

	void Profiler::Init(const char* fileName)
	{
		mtr_init(fileName);
	}

	void Profiler::Stop()
	{
		mtr_flush();
		mtr_shutdown();
	}

	void Profiler::BeginTrace(const char* category, const char* name)
	{
		MTR_BEGIN(category, name);
	}

	void Profiler::EndTrace(const char* category, const char* name)
	{
		MTR_END(category, name);
	}

	struct StatHistory
	{
		static const uint32 kHistorySize = 64;
		Array<double, kHistorySize> deltasHistory;
		uint32 currDelta = 0;

		double tMax;
		double tAvg;

		void RecordStat(double t)
		{
			deltasHistory[currDelta] = t;
			currDelta = (currDelta + 1) % StatHistory::kHistorySize;

			tAvg = 0;
			tMax = 0;
			uint32 validSamples = 0;
			for (double v : deltasHistory)
			{
				if (v > 0.0)
				{ 
					++validSamples;
					tAvg += v;
					if (v > tMax)
					{
						tMax = v;
					}
				}
			}

			if (validSamples > 0)
			{
				tAvg /= double(validSamples);
			}
		}
	};

	enum class ProfileState
	{
		IDLE = 0,
		ACTIVE,
		FINISHED,
	};

	struct ProfileBlock
	{
		std::string name;
		ProfileState state = ProfileState::IDLE;

		gfx::GraphicsContext* ctx = nullptr;
		uint32 timestampIdx = 0;

		int64 tStart = 0;
		int64 tEnd = 0;

		StatHistory cpuStats;
		StatHistory gpuStats;
	};

	static const uint32 kMaxProfiles = gfx::NUM_TIMESTAMP_QUERIES / 2;
	static Array<ProfileBlock, kMaxProfiles> s_Profiles;
	static uint32 s_ProfileCount = 0;
	static Timer s_Timer;

	static ProfileBlock& FindOrAddProfile(const char* name, bool& bFound)
	{
		for (uint32 i = 0; i < s_ProfileCount; ++i)
		{
			if (s_Profiles[i].name == name)
			{
				bFound = true;
				return s_Profiles[i];
			}
		}
		VAST_ASSERTF(s_ProfileCount < kMaxProfiles, "Exceeded max number of profiles ({}).", kMaxProfiles);
		bFound = false;
		return s_Profiles[s_ProfileCount++];
	}

	void Profiler::PushProfilingMarker(const char* name, gfx::GraphicsContext* ctx /* = nullptr */)
	{
		bool bFound;
		ProfileBlock& p = FindOrAddProfile(name, bFound);
		if (!bFound)
		{
			p.name = name;
			p.ctx = ctx;
		}
		VAST_ASSERTF(ctx == p.ctx, "Changing execution context of an existing profile is not allowed.");
		VAST_ASSERTF(p.state == ProfileState::IDLE, "A profile of this name already exists or has already been pushed this frame.")

		// CPU Timing
		s_Timer.Update();
		p.tStart = s_Timer.GetElapsedMicroseconds<int64>();

		if (ctx)
		{
			// GPU Timing
			p.timestampIdx = p.ctx->BeginTimestamp();
		}

		p.state = ProfileState::ACTIVE;
	}

	static ProfileBlock* FindLastActiveEntry()
	{
		VAST_ASSERT(s_ProfileCount > 0);
		for (int32 i = (s_ProfileCount - 1); i >= 0; --i)
		{
			if (s_Profiles[i].state == ProfileState::ACTIVE)
			{
				return &s_Profiles[i];
			}
		}

		VAST_ASSERTF(0, "Active profile not found.");
		return nullptr;
	}

	void Profiler::PopProfilingMarker()
	{
		if (ProfileBlock* p = FindLastActiveEntry())
		{
			// CPU Timing
			s_Timer.Update();
			p->tEnd = s_Timer.GetElapsedMicroseconds<int64>();

			if (p->ctx)
			{
				// GPU Timing
				p->ctx->EndTimestamp(p->timestampIdx);
			}
			p->state = ProfileState::FINISHED;
		}
	}

	void Profiler::UpdateProfiles()
	{
		if (s_ProfileCount == 0)
			return;

		for (uint32 i = 0; i < s_ProfileCount; ++i)
		{
			ProfileBlock& p = s_Profiles[i];
			VAST_ASSERTF(p.state == ProfileState::FINISHED, "Attempted to update a profile that is still active.")

			p.cpuStats.RecordStat(double(p.tEnd - p.tStart) / 1000.0);
			if (p.ctx)
			{
				p.gpuStats.RecordStat(p.ctx->GetTimestampDuration(p.timestampIdx) * 1000.0);
			}

			p.state = ProfileState::IDLE;
		}
	}

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

			ImGui::Text("CPU Profiler");
			ImGui::Separator();

			for (uint32 i = 0; i < s_ProfileCount; ++i)
			{
				ProfileBlock& p = s_Profiles[i];
				StatHistory& s = s_Profiles[i].cpuStats;

				ImGui::Text("%s: %.3fms (%.3fms max)", p.name.c_str(), s.tAvg, s.tMax);
			}

			ImGui::Text("\nGPU Profiler");
			ImGui::Separator();

			for (uint32 i = 0; i < s_ProfileCount; ++i)
			{
				ProfileBlock& p = s_Profiles[i];

				if (!p.ctx)
					continue;

				StatHistory& s = s_Profiles[i].gpuStats;

				ImGui::Text("%s: %.3fms (%.3fms max)", p.name.c_str(), s.tAvg, s.tMax);
			}
		}
		ImGui::End();
	}

}
