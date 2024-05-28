#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_CPU_SCOPE(n)		vast::ProfileScopeCPU XCAT(_profCpu, __LINE__)(n)
#define VAST_PROFILE_CPU_BEGIN(n)		vast::Profiler::PushProfilingMarkerCPU(n)
#define VAST_PROFILE_CPU_END()			vast::Profiler::PopProfilingMarkerCPU()

#define VAST_PROFILE_GPU_SCOPE(n, ctx)	vast::ProfileScopeGPU XCAT(_profGpu, __LINE__)(n, ctx.GetGPUProfiler())
#define VAST_PROFILE_GPU_BEGIN(n, ctx)	vast::Profiler::PushProfilingMarkerGPU(n, ctx.GetGPUProfiler())
#define VAST_PROFILE_GPU_END()			vast::Profiler::PopProfilingMarkerGPU(ctx.GetGPUProfiler())
// TODO: Single profile macro, select cpu/gpu
#else
#define VAST_PROFILE_CPU_SCOPE(n)	
#define VAST_PROFILE_CPU_BEGIN(n)	
#define VAST_PROFILE_CPU_END()		

#define VAST_PROFILE_GPU_SCOPE(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)
#define VAST_PROFILE_GPU_END()
#endif

namespace vast
{
	namespace gfx
	{
		class GraphicsContext;
		class GPUProfiler;
	}

	class Profiler
	{
		friend class WindowedApp;
	public:
		static void PushProfilingMarkerCPU(const char* name);
		static void PushProfilingMarkerGPU(const char* name, gfx::GPUProfiler& gpuProfiler);
		static void PopProfilingMarkerCPU();
		static void PopProfilingMarkerGPU(gfx::GPUProfiler& gpuProfiler);

		static void FlushProfiles();

		static void OnGUI();
		static void DrawTextMinimal();
		static float GetTextMinimalLength();
	private:
		static void Init();

		static void BeginFrame();
		static void EndFrame(gfx::GraphicsContext& ctx);
	};

	struct ProfileScopeCPU
	{
		ProfileScopeCPU(const char* name)
		{
			Profiler::PushProfilingMarkerCPU(name);
		}

		~ProfileScopeCPU()
		{
			Profiler::PopProfilingMarkerCPU();
		}
	};

	struct ProfileScopeGPU
	{
		ProfileScopeGPU(const char* name, gfx::GPUProfiler& gpuProfiler_) : gpuProfiler(gpuProfiler_)
		{
			Profiler::PushProfilingMarkerGPU(name, gpuProfiler);
		}

		~ProfileScopeGPU()
		{
			Profiler::PopProfilingMarkerGPU(gpuProfiler);
		}

	private:
		gfx::GPUProfiler& gpuProfiler;
	};
}