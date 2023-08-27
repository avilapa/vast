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



	struct ProfileBlock
	{
		std::string name;

		enum class State
		{
			IDLE = 0,
			ACTIVE,
			FINISHED,
		} state = State::IDLE;

		ProfileBlock* parent = nullptr;
		uint32 childCount = 0;
		uint32 treeDepth = 0;

		StatHistory stats;

		// CPU specific
		int64 tBegin = 0;
		int64 tEnd = 0;
		// GPU specific
		gfx::GraphicsContext* ctx = nullptr;
		uint32 timestampIdx = 0;
	};

	//

	static Timer s_Timer;

	//

	template<typename ProfileBlockArray>
	static ProfileBlock& FindOrAddProfile(ProfileBlockArray& profiles, uint32& profileCount, const char* name, bool& bFound)
	{
		for (uint32 i = 0; i < profileCount; ++i)
		{
			if (profiles[i].name == name)
			{
				bFound = true;
				return profiles[i];
			}
		}
		VAST_ASSERTF(profileCount < profiles.size(), "Exceeded max number of profiles ({}).", profiles.size());
		bFound = false;
		return profiles[profileCount++];
	}

	template<typename ProfileBlockArray>
	static ProfileBlock* FindLastActiveEntry(ProfileBlockArray& profiles, const uint32 profileCount)
	{
		VAST_ASSERT(profileCount > 0);
		for (int32 i = (profileCount - 1); i >= 0; --i)
		{
			if (profiles[i].state == ProfileBlock::State::ACTIVE)
			{
				return &profiles[i];
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

	template<typename ProfileBlockArray>
	static void PushProfilingMarker_Internal(ProfileBlockArray& profiles, uint32& profileCount, const char* name, gfx::GraphicsContext* ctx, bool bIsGPU)
	{
		bool bFound;
		ProfileBlock& p = FindOrAddProfile(profiles, profileCount, name, bFound);
		if (!bFound)
		{
			p.name = name;
			p.ctx = ctx;
		}
		VAST_ASSERTF(p.state == ProfileBlock::State::IDLE, "A profile named '{}' already exists or has already been pushed this frame.", p.name);
		VAST_ASSERTF((p.ctx != nullptr) == bIsGPU, "Mismatched profiling context not allowed.");

		if (p.ctx)
		{
			VAST_ASSERT(p.ctx);
			p.timestampIdx = p.ctx->BeginTimestamp();
		}
		else
		{
			s_Timer.Update();
			p.tBegin = s_Timer.GetElapsedMicroseconds<int64>();
		}

		// Find tree position
		if (p.parent = FindLastActiveEntry(profiles, profileCount))
		{
			p.parent->childCount++;
		}
		p.treeDepth = FindTreeDepth(&p);

		p.state = ProfileBlock::State::ACTIVE;
	}

	template<typename ProfileBlockArray>
	static void PopProfilingMarker_Internal(ProfileBlockArray& profiles, const uint32 profileCount, bool bIsGPU)
	{
		if (ProfileBlock* p = FindLastActiveEntry(profiles, profileCount))
		{
			VAST_ASSERTF((p->ctx != nullptr) == bIsGPU, "Mismatched profiling context not allowed.");
			if (p->ctx)
			{
				p->ctx->EndTimestamp(p->timestampIdx);
			}
			else
			{
				s_Timer.Update();
				p->tEnd = s_Timer.GetElapsedMicroseconds<int64>();
			}
			p->state = ProfileBlock::State::FINISHED;
		}
		else
		{
			VAST_ASSERTF(0, "Active profile not found.");
		}
	}

	//

	// CPU Profiles
	static Array<ProfileBlock, 128> s_CpuProfiles;
	static uint32 s_CpuProfileCount = 0;
	static StatHistory s_CpuProfile;

	// GPU Profiles
	static Array<ProfileBlock, (gfx::NUM_TIMESTAMP_QUERIES / 2)> s_GpuProfiles;
	static uint32 s_GpuProfileCount = 0;
	static StatHistory s_GpuProfile;

	// General
	static int64 s_tFrameStart;
	static StatHistory s_FrameProfile;
	static bool s_bShowProfiler = false;
	static bool s_bProfilesNeedFlush = false;

	//

	void Profiler::PushProfilingMarkerCPU(const char* name)
	{
		PushProfilingMarker_Internal(s_CpuProfiles, s_CpuProfileCount, name, nullptr, false);
	}

	void Profiler::PushProfilingMarkerGPU(const char* name, gfx::GraphicsContext* ctx)
	{
		VAST_ASSERT(ctx);
		PushProfilingMarker_Internal(s_GpuProfiles, s_GpuProfileCount, name, ctx, true);
	}

	void Profiler::PopProfilingMarkerCPU()
	{
		PopProfilingMarker_Internal(s_CpuProfiles, s_CpuProfileCount, false);
	}

	void Profiler::PopProfilingMarkerGPU()
	{
		PopProfilingMarker_Internal(s_GpuProfiles, s_GpuProfileCount, true);
	}

	void Profiler::FlushProfiles()
	{
		s_bProfilesNeedFlush = true;
	}

	void Profiler::Init()
	{
		VAST_LOG_INFO("[profiler] Initializing Profiler...");
		VAST_SUBSCRIBE_TO_EVENT("profiler", DebugActionEvent, VAST_EVENT_HANDLER_EXP_STATIC(s_bShowProfiler = !s_bShowProfiler));
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
			auto ResetProfiles = [](auto& profiles, uint32& profileCount)
			{
				for (uint32 i = 0; i < profileCount; ++i)
				{
					profiles[i] = ProfileBlock{};
				}
				profileCount = 0;
			};

			ResetProfiles(s_CpuProfiles, s_CpuProfileCount);
			ResetProfiles(s_GpuProfiles, s_GpuProfileCount);

			s_bProfilesNeedFlush = false;

			// Note: Disregard updating profiles if user signaled a flush.
			return;
		}

		auto UpdateProfilesStats = [](auto& profiles, const uint32 profileCount, bool bIsGPU)
		{
			for (uint32 i = 0; i < profileCount; ++i)
			{
				ProfileBlock& p = profiles[i];
				VAST_ASSERTF(p.state == ProfileBlock::State::FINISHED, "Attempted to update a profile that is still active.");
				VAST_ASSERTF((p.ctx != nullptr) == bIsGPU, "Mismatched profiling context not allowed.");

				if (p.ctx)
				{
					p.stats.RecordStat(p.ctx->GetTimestampDuration(p.timestampIdx) * 1000.0);
				}
				else
				{
					p.stats.RecordStat(double(p.tEnd - p.tBegin) / 1000.0);
				}

				p.state = ProfileBlock::State::IDLE;
			}
		};

		UpdateProfilesStats(s_CpuProfiles, s_CpuProfileCount, false);
		UpdateProfilesStats(s_GpuProfiles, s_GpuProfileCount, true);
	}

	//

	template<typename ProfileBlockArray>
	static void DrawNestedProfilesTable(ProfileBlockArray& profiles, const uint32 profileCount)
	{
		ImGui::BeginTable("Profiling", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Time");
		ImGui::TableHeadersRow();

		uint32 currTreeDepth = 0;
		for (uint32 i = 0; i < profileCount; ++i)
		{
			ProfileBlock& p = profiles[i];

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
					for (uint32 j = (i + 1); j < profileCount; ++j)
					{
						if (profiles[j].treeDepth > currTreeDepth)
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
			ImGui::Text("%.3fms", p.stats.tAvg);
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
				DrawNestedProfilesTable(s_GpuProfiles, s_GpuProfileCount);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CPU Timings"))
			{
				DrawNestedProfilesTable(s_CpuProfiles, s_CpuProfileCount);
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
