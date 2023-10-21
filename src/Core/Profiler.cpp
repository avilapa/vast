#include "vastpch.h"
#include "Core/Profiler.h"
#include "Core/Types.h"
#include "Core/Timer.h"
#include "Graphics/GraphicsContext.h"
#include "Rendering/Imgui.h"

#include "minitrace/minitrace.h"

#include <queue>
#include <deque>

namespace vast
{

	struct StatHistory
	{
		static const uint32 kHistorySize = 64;
		Array<double, kHistorySize> history;

		double tMax = 0;
		double tAvg = 0;
		double tLast = 0;

		void RecordTimeLast(double t)
		{
			history[currSample] = tLast = t;
			currSample = (currSample + 1) % StatHistory::kHistorySize;
		}

		void UpdateAverages()
		{
			tAvg = 0;
			tMax = 0;
			uint32 validSamples = 0;
			for (double v : history)
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

	private:
		uint32 currSample = 0;
	};

	struct PlotHistory
	{
		static const uint32 kHistorySize = 512;
		Array<float, kHistorySize> history;

		float tMin = FLT_MAX;
		float tMax = 0;

		void RecordTimeLast(float tLast)
		{
			// Shift plot data to the left and update last entry.
			memmove(history.data(), history.data() + 1, sizeof(float) * (kHistorySize - 1));
			history[kHistorySize - 1] = tLast;

			if (tLast > tMax)
				tMax = tLast;
			if (tLast < tMin)
				tMin = tLast;
		}

		void ResetMinMax()
		{
			tMin = FLT_MAX;
			tMax = 0;
			for (float v : history)
			{
				if (v > 0.0)
				{
					if (v > tMax)
						tMax = v;
					if (v < tMin)
						tMin = v;
				}
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
		p.parent = FindLastActiveEntry(profiles, profileCount);
		if (p.parent)
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
	static StatHistory s_CpuStats;
	static PlotHistory s_CpuPlot;

	// GPU Profiles
	static Array<ProfileBlock, (gfx::NUM_TIMESTAMP_QUERIES / 2)> s_GpuProfiles;
	static uint32 s_GpuProfileCount = 0;
	static StatHistory s_GpuStats;
	static PlotHistory s_GpuPlot;

	// General
	static float s_tLastStatsUpdate = 0.0f;
	static float s_StatUpdateFrequencySeconds = 0.1f;
	static float s_tLastPlotsMaxReset = 0.0f;
	static float s_PlotMaxResetFrequencySeconds = 5.0f;

	static int64 s_tFrameStart;
	static StatHistory s_FrameStats;
	static PlotHistory s_FramePlot;

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

		// Record global stats, and update plots (if user interface is showing)
		{
			double durationMs = double(s_Timer.GetElapsedMicroseconds<int64>() - s_tFrameStart) / 1000.0;
			s_FrameStats.RecordTimeLast(durationMs);
			if (s_bShowProfiler) s_FramePlot.RecordTimeLast(static_cast<float>(durationMs));
		}
		{
			double durationMs = ctx.GetLastFrameDuration() * 1000.0;
			s_GpuStats.RecordTimeLast(durationMs);
			if (s_bShowProfiler) s_GpuPlot.RecordTimeLast(static_cast<float>(durationMs));
		}

		float tTimeNowSeconds = s_Timer.GetElapsedSeconds<float>();
		// Check if we should update stats this frame (i.e. recompute averages).
		bool bStatsNeedUpdateThisFrame = false;
		if ((tTimeNowSeconds - s_tLastStatsUpdate) >= s_StatUpdateFrequencySeconds)
		{
			bStatsNeedUpdateThisFrame = true;
			s_tLastStatsUpdate = tTimeNowSeconds;

			s_FrameStats.UpdateAverages();
			s_GpuStats.UpdateAverages();
		}
		// Check if we should reset min/max value on plots this frame.
		if (s_bShowProfiler && (tTimeNowSeconds - s_tLastPlotsMaxReset) >= s_PlotMaxResetFrequencySeconds)
		{
			s_tLastPlotsMaxReset = tTimeNowSeconds;

			s_FramePlot.ResetMinMax();
			s_GpuPlot.ResetMinMax();
		}

		// Reset user profiles if needed.
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

			// Disregard updating profiles if user signaled a flush.
			return;
		}

		// Record and update (if needed) user profile stats
		auto UpdateProfilesStats = [&](auto& profiles, const uint32 profileCount, bool bIsGPU)
		{
			for (uint32 i = 0; i < profileCount; ++i)
			{
				ProfileBlock& p = profiles[i];

				// TODO: How do we treat profiles that run only once?
				if (p.state == ProfileBlock::State::IDLE)
					continue;

				VAST_ASSERTF(p.state != ProfileBlock::State::ACTIVE, "Attempted to update a profile that is still active.");
				VAST_ASSERTF((p.ctx != nullptr) == bIsGPU, "Mismatched profiling context not allowed.");

				if (p.ctx)
				{
					p.stats.RecordTimeLast(p.ctx->GetTimestampDuration(p.timestampIdx) * 1000.0);
				}
				else
				{
					p.stats.RecordTimeLast(double(p.tEnd - p.tBegin) / 1000.0);
				}

				if (bStatsNeedUpdateThisFrame)
				{
					p.stats.UpdateAverages();
				}

				p.state = ProfileBlock::State::IDLE;
			}
		};

		UpdateProfilesStats(s_CpuProfiles, s_CpuProfileCount, false);
		UpdateProfilesStats(s_GpuProfiles, s_GpuProfileCount, true);
	}

	//

	static bool s_bWindowAutoResize = true;
	static bool s_bWindowAllowMoving = false;
	static bool s_bDisplayTotals = true;

	//

	template<typename ProfileBlockArray>
	static void DrawNestedProfilesTable(ProfileBlockArray& profiles, const uint32 profileCount, const StatHistory& total)
	{
		const ImGuiTreeNodeFlags treeBranchFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;
		const ImGuiTreeNodeFlags treeLeafFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		struct BranchStats
		{
			double totalAvg;
			double untrackedAvg;
		};
		std::queue<BranchStats> branchStats;
		BranchStats rootStats = { 0.0, 0.0 };

		auto DrawBranchStats = [](const BranchStats& branch)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::TreeNodeEx("Tracked Total", treeLeafFlags);
			ImGui::TableNextColumn();
			ImGui::Text("%.3f ms", branch.totalAvg);
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Untracked (Avg): %.3f", branch.untrackedAvg);
			}
		};

		auto DoTreePops = [&](uint32 diff)
		{
			while (diff-- > 0)
			{
				if (s_bDisplayTotals)
				{
					// Display total average time at the end of children
					DrawBranchStats(branchStats.front());
					branchStats.pop();
				}
				ImGui::TreePop();
			}
		};

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
				DoTreePops(diff);
			}
			currTreeDepth = p.treeDepth;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (p.childCount > 0)
			{
				bool bOpen = ImGui::TreeNodeEx(p.name.c_str(), treeBranchFlags);

				if (bOpen)
				{
					if (s_bDisplayTotals)
					{
						// Sum up averages for current tree depth
						double currTotalAvg = 0;
						uint32 currChildrenTreeDepth = p.treeDepth + 1;
						for (uint32 j = (i + 1); j < profileCount; ++j)
						{
							ProfileBlock& child = profiles[j];
							if (child.treeDepth == currChildrenTreeDepth)
							{
								currTotalAvg += child.stats.tAvg;
							}
							else if (child.treeDepth <= currTreeDepth) // TODO: Check childCount?
							{
								break;
							}
						}
						double currUntrackedAvg = std::max(0.0, p.stats.tAvg - currTotalAvg);
						branchStats.push({ currTotalAvg, currUntrackedAvg });
					}
				}
				else
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
				ImGui::TreeNodeEx(p.name.c_str(), treeLeafFlags);
			}

			ImGui::TableNextColumn();
			ImGui::Text("%.3f ms", p.stats.tAvg);
			if (currTreeDepth == 0)
			{
				rootStats.totalAvg += p.stats.tAvg;
			}
		}

		// Pop any unpopped indentation and draw totals
		DoTreePops(currTreeDepth);
		if (s_bDisplayTotals)
		{
			rootStats.untrackedAvg = std::max(0.0, total.tAvg - rootStats.totalAvg);
			DrawBranchStats(rootStats);
		}

		ImGui::EndTable();
	}

	static void DrawPlotHistory(const PlotHistory& plot, double avg, double max)
	{
		char overlay[64];
		sprintf_s(overlay, "avg %0.3f ms (max %0.3f ms)", avg, max);
		float availableWidth = ImGui::GetContentRegionAvail().x;
		ImGui::PlotLines("", plot.history.data(), static_cast<int>(plot.kHistorySize), 0, overlay, std::min(plot.tMin, std::max(float(avg) - 1.5f, 0.0f)), std::max(plot.tMax, float(avg) + 1.5f), ImVec2(availableWidth, 80.0f));
	}

	void Profiler::OnGUI()
	{
		if (!s_bShowProfiler)
			return;

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

		ImGui::Text("Frame: %.3f ms (CPU: %.3f ms, GPU: %.3f ms)", s_FrameStats.tAvg, s_CpuStats.tAvg, s_GpuStats.tAvg);

		if (ImGui::BeginTabBar("##ProfilerTabBar"))
		{
			if (ImGui::BeginTabItem("Frame Timings"))
			{
				DrawPlotHistory(s_FramePlot, s_FrameStats.tAvg, s_FrameStats.tMax);
				ImGui::Separator();
				// TODO: FPS
				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("CPU Timings"))
			{
				DrawPlotHistory(s_CpuPlot, s_CpuStats.tAvg, s_CpuStats.tMax);
				ImGui::Separator();
				DrawNestedProfilesTable(s_CpuProfiles, s_CpuProfileCount, s_CpuStats);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("GPU Timings"))
			{
				DrawPlotHistory(s_GpuPlot, s_GpuStats.tAvg, s_GpuStats.tMax);
				ImGui::Separator();
				DrawNestedProfilesTable(s_GpuProfiles, s_GpuProfileCount, s_GpuStats);
				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Settings"))
			{
				if (ImGui::TreeNodeEx("Window", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Auto-resize enabled", &s_bWindowAutoResize);
					ImGui::Checkbox("Allow moving", &s_bWindowAllowMoving);
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Display", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::SliderFloat("Time update frequency (seconds)", &s_StatUpdateFrequencySeconds, 0.0f, 1.0f);
					ImGui::Checkbox("Display Totals at each level", &s_bDisplayTotals);
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
		ImGui::Text("Frame: %.3f ms (GPU: %.3f ms)", s_FrameStats.tAvg, s_GpuStats.tAvg);
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
