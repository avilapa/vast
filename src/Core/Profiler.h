#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_CPU_SCOPE(n)		::vast::profile::ScopedCPUProf XCAT(_profCpuVar, __LINE__)(n)
#define VAST_PROFILE_CPU_BEGIN(n)		::vast::profile::PushProfilingMarkerCPU(n)
#define VAST_PROFILE_CPU_END()			::vast::profile::PopProfilingMarkerCPU()

#define VAST_PROFILE_GPU_SCOPE(n, ctx)	::vast::profile::ScopedGPUProf XCAT(_profGpuVar, __LINE__)(n, ctx.GetGPUProfiler())
#define VAST_PROFILE_GPU_BEGIN(n, ctx)	::vast::profile::PushProfilingMarkerGPU(n, ctx.GetGPUProfiler())
#define VAST_PROFILE_GPU_END()			::vast::profile::PopProfilingMarkerGPU(ctx.GetGPUProfiler())
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

	namespace profile
	{
		void BeginFrame();
		void EndFrame(gfx::GraphicsContext& ctx);

		// Resets all CPU and GPU profiles and recorded history.
		void FlushProfiles();

		void PushProfilingMarkerCPU(const char* name);
		void PushProfilingMarkerGPU(const char* name, gfx::GPUProfiler& gpuProfiler);
		void PopProfilingMarkerCPU();
		void PopProfilingMarkerGPU(gfx::GPUProfiler& gpuProfiler);

		struct ScopedCPUProf
		{
			ScopedCPUProf(const char* name) { PushProfilingMarkerCPU(name); }
			~ScopedCPUProf() { PopProfilingMarkerCPU(); }
		};

		struct ScopedGPUProf
		{
			ScopedGPUProf(const char* name, gfx::GPUProfiler& gpuProfiler) : m_GpuProfiler(gpuProfiler) { PushProfilingMarkerGPU(name, m_GpuProfiler); }
			~ScopedGPUProf() { PopProfilingMarkerGPU(m_GpuProfiler); }

		private:
			gfx::GPUProfiler& m_GpuProfiler;
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