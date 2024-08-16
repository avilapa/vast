#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_CPU_SCOPE(n)		::vast::Profiler::ScopedCPUProf XCAT(_profCpuVar, __LINE__)(n)
#define VAST_PROFILE_CPU_BEGIN(n)		::vast::Profiler::PushProfilingMarkerCPU(n)
#define VAST_PROFILE_CPU_END()			::vast::Profiler::PopProfilingMarkerCPU()

#define VAST_PROFILE_GPU_SCOPE(n, ctx)	::vast::Profiler::ScopedGPUProf XCAT(_profGpuVar, __LINE__)(n, ctx.GetGPUProfiler())
#define VAST_PROFILE_GPU_BEGIN(n, ctx)	::vast::Profiler::PushProfilingMarkerGPU(n, ctx.GetGPUProfiler())
#define VAST_PROFILE_GPU_END()			::vast::Profiler::PopProfilingMarkerGPU(ctx.GetGPUProfiler())
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
	class GraphicsContext;
	class GPUProfiler;

	namespace Profiler
	{
		void BeginFrame();
		void EndFrame(GraphicsContext& ctx);

		// Resets all CPU and GPU profiles and recorded history.
		void FlushProfiles();

		void PushProfilingMarkerCPU(const char* name);
		void PushProfilingMarkerGPU(const char* name, GPUProfiler& gpuProfiler);
		void PopProfilingMarkerCPU();
		void PopProfilingMarkerGPU(GPUProfiler& gpuProfiler);

		struct ScopedCPUProf
		{
			ScopedCPUProf(const char* name) { PushProfilingMarkerCPU(name); }
			~ScopedCPUProf() { PopProfilingMarkerCPU(); }
		};

		struct ScopedGPUProf
		{
			ScopedGPUProf(const char* name, GPUProfiler& gpuProfiler) : m_GpuProfiler(gpuProfiler) { PushProfilingMarkerGPU(name, m_GpuProfiler); }
			~ScopedGPUProf() { PopProfilingMarkerGPU(m_GpuProfiler); }

		private:
			GPUProfiler& m_GpuProfiler;
		};

		namespace ui
		{
			void OnGUI();
			void DrawTextMinimal();
			float GetTextMinimalLength();

			extern bool g_bShowProfiler;
		}
	}

}