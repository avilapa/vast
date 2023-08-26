#include "vastpch.h"
#include "Core/Profiler.h"
#include "Core/Types.h"
#include "Core/Timer.h"
#include "Graphics/GraphicsContext.h"

#include "minitrace/minitrace.h"
#include "imgui/imgui.h"

namespace vast
{

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

		ProfileBlock* parent = nullptr;
		uint32 childCount = 0;
		uint32 treeDepth = 0;
	};

	//

	static const uint32 kMaxProfiles = gfx::NUM_TIMESTAMP_QUERIES / 2;
	static Array<ProfileBlock, kMaxProfiles> s_Profiles;
	static uint32 s_ProfileCount = 0;

	static Timer s_Timer;
	static bool s_bShowProfiler = false;
	static bool s_bProfilesNeedFlush = false;

	static int64 s_tFrameStart;
	static StatHistory s_FrameProfile;
	static StatHistory s_CpuProfile;
	static StatHistory s_GpuProfile;

	//

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
		VAST_ASSERTF(s_ProfileCount < s_Profiles.size(), "Exceeded max number of profiles ({}).", s_Profiles.size());
		bFound = false;
		return s_Profiles[s_ProfileCount++];
	}

	static ProfileBlock* FindLastActiveEntry()
	{
		VAST_ASSERT(s_ProfileCount > 0);
		for (int32 i = (s_ProfileCount - 1); i >= 0; --i)
		{
			// TODO: Should we check that it executes on the same context as well?
			if (s_Profiles[i].state == ProfileState::ACTIVE)
			{
				return &s_Profiles[i];
			}
		}
		return nullptr;
	}

	static uint32 FindTreeDepth(const ProfileBlock* p, uint32 currDepth = 0)
	{
		if (p && p->parent)
		{
			currDepth = FindTreeDepth(p->parent, ++currDepth);
		}
		return currDepth;
	}

	//

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
		VAST_ASSERTF(p.state == ProfileState::IDLE, "A profile of this name already exists or has already been pushed this frame.");

		// CPU Timing
		s_Timer.Update();
		p.tStart = s_Timer.GetElapsedMicroseconds<int64>();

		if (ctx)
		{
			// GPU Timing
			p.timestampIdx = p.ctx->BeginTimestamp();
		}

		// Find tree position
		if (p.parent = FindLastActiveEntry())
		{
			p.parent->childCount++;
		}
		p.treeDepth = FindTreeDepth(&p);

		p.state = ProfileState::ACTIVE;
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
		else
		{
			VAST_ASSERTF(0, "Active profile not found.");
		}
	}

	void Profiler::FlushProfiles()
	{
		s_bProfilesNeedFlush = true;
	}

	void Profiler::BeginFrame()
	{
		s_Timer.Update();
		s_tFrameStart = s_Timer.GetElapsedMicroseconds<int64>();
	}

	void Profiler::EndFrame(gfx::GraphicsContext& ctx)
	{
		s_Timer.Update();
		int64 tFrameEnd = s_Timer.GetElapsedMicroseconds<int64>();
		// Update frame stats
		s_FrameProfile.RecordStat(double(tFrameEnd - s_tFrameStart) / 1000.0);
		s_GpuProfile.RecordStat(ctx.GetLastFrameDuration() * 1000.0);

		if (s_bProfilesNeedFlush)
		{
			for (uint32 i = 0; i < s_ProfileCount; ++i)
			{
				s_Profiles[i] = ProfileBlock{};
			}
			s_ProfileCount = 0;
			s_bProfilesNeedFlush = false;

			// Note: Disregard updating profiles if user signaled a flush.
			return;
		}

		// Update profiles stats
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

	void Profiler::Init()
	{
		VAST_LOG_INFO("[profiler] Initializing Profiler...");
		VAST_SUBSCRIBE_TO_EVENT("profiler", DebugActionEvent, VAST_EVENT_HANDLER_EXP_STATIC(s_bShowProfiler = !s_bShowProfiler));
	}

	//

	static void DrawNestedProfilesTable()
	{
		ImGui::BeginTable("Profiling", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("CPU Time");
		ImGui::TableSetupColumn("GPU Time");
		ImGui::TableHeadersRow();

		uint32 currTreeDepth = 0;
		for (uint32 i = 0; i < s_ProfileCount; ++i)
		{
			ProfileBlock& p = s_Profiles[i];

			// Check if we moved up on the tree
			if (currTreeDepth > 0 && p.treeDepth < currTreeDepth)
			{
				uint32 diff = currTreeDepth - p.treeDepth;
				while (diff-- > 0)
				{
					ImGui::TreePop();
				}
			}
			currTreeDepth = p.treeDepth;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (p.childCount > 0)
			{
				ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_DefaultOpen
									 | ImGuiTreeNodeFlags_SpanFullWidth;

				if (!ImGui::TreeNodeEx(p.name.c_str(), f))
				{
					// If parent is closed, skip all children and their children...
					for (uint32 j = (i + 1); j < s_ProfileCount; ++j)
					{
						if (s_Profiles[j].treeDepth > currTreeDepth)
						{
							++i;
						}
					}
				}
			}
			else
			{
				ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_Leaf
									 | ImGuiTreeNodeFlags_Bullet
									 | ImGuiTreeNodeFlags_NoTreePushOnOpen
									 | ImGuiTreeNodeFlags_SpanFullWidth;

				ImGui::TreeNodeEx(p.name.c_str(), f);
			}
			ImGui::TableNextColumn();
			ImGui::Text("%.3fms", p.cpuStats.tAvg);
			ImGui::TableNextColumn();
			if (p.ctx)
			{
				ImGui::Text("%.3fms", p.gpuStats.tAvg);
			}
		}

		if (currTreeDepth > 0)
		{
			while (currTreeDepth-- > 0)
			{
				ImGui::TreePop();
			}
		}

		ImGui::EndTable();
	}

	void Profiler::OnGUI()
	{
		if (!s_bShowProfiler)
			return;

		static bool s_bWindowAutoResize = true;
		static bool s_bWindowAllowMoving = false;

		if (!s_bWindowAllowMoving)
		{
			const float pad = 10.0f;
			const ImGuiViewport* v = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(ImVec2(v->WorkPos.x + v->WorkSize.x - pad, v->WorkPos.y + pad), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		}

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings;
		if (s_bWindowAutoResize) windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

		if (!ImGui::Begin("Profiler", 0, windowFlags))
		{
			ImGui::End();
			return;
		}

		ImGui::Text("Frame: %.3f ms (CPU: %.3f ms, GPU: %.3f ms)", s_FrameProfile.tAvg, s_CpuProfile.tAvg, s_GpuProfile.tAvg);

		if (ImGui::BeginTabBar("##ProfilerTabBar"))
		{
			if (ImGui::BeginTabItem("GPU Timings"))
			{
				for (uint32 i = 0; i < s_ProfileCount; ++i)
				{
					ProfileBlock& p = s_Profiles[i];

					if (!p.ctx)
						continue;

					StatHistory& s = s_Profiles[i].gpuStats;

					ImGui::Text("%s: %.3fms (%.3fms max)", p.name.c_str(), s.tAvg, s.tMax);
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CPU Timings"))
			{
				for (uint32 i = 0; i < s_ProfileCount; ++i)
				{
					ProfileBlock& p = s_Profiles[i];
					StatHistory& s = s_Profiles[i].cpuStats;

					ImGui::Text("%s: %.3fms (%.3fms max)", p.name.c_str(), s.tAvg, s.tMax);
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Nested View"))
			{
				DrawNestedProfilesTable();
				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Settings"))
			{
				if (ImGui::TreeNode("Window"))
				{
					ImGui::Checkbox("Auto-resize enabled", &s_bWindowAutoResize);
					ImGui::Checkbox("Allow moving", &s_bWindowAllowMoving);
					ImGui::TreePop();
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void Profiler::DrawTextMinimal()
	{
		ImGui::Text("Frame: %.3f ms (GPU: %.3f ms)", s_FrameProfile.tAvg, s_GpuProfile.tAvg);
	}
	
	float Profiler::GetTextMinimalLength()
	{
		return ImGui::CalcTextSize("Frame: 0.000f ms (GPU: 0.000f ms)").x;
	}

	//

	void TraceLogger::BeginTrace(const char* category, const char* name)
	{
		MTR_BEGIN(category, name);
	}

	void TraceLogger::EndTrace(const char* category, const char* name)
	{
		MTR_END(category, name);
	}

	void TraceLogger::Init(const char* fileName)
	{
#if VAST_ENABLE_TRACING
		mtr_init(fileName);
#endif
	}

	void TraceLogger::Stop()
	{
#if VAST_ENABLE_TRACING
		mtr_flush();
		mtr_shutdown();
#endif
	}

}
