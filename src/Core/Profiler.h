#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_CPU_SCOPE(n)		vast::Profiler::ScopedCpuTiming  XCAT(_profCpu, __LINE__)(n)
#define VAST_PROFILE_CPU_BEGIN(n)		vast::Profiler::BeginCpuTiming(n)
#define VAST_PROFILE_CPU_END()			vast::Profiler::EndCpuTiming()

#define VAST_PROFILE_GPU_SCOPE(n, ctx)	vast::Profiler::ScopedGpuTiming  XCAT(_profGpu, __LINE__)(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)	vast::Profiler::BeginGpuTiming(n, ctx)
#define VAST_PROFILE_GPU_END(ctx)		vast::Profiler::EndGpuTiming(ctx)

#define VAST_PROFILE_TRACE_SCOPE(c, n)	vast::Profiler::ScopedTrace XCAT(_profTrace, __LINE__)(c, n)
#define VAST_PROFILE_TRACE_BEGIN(c, n)	vast::Profiler::BeginTrace(c, n)
#define VAST_PROFILE_TRACE_END(c, n)	vast::Profiler::EndTrace(c, n)
#else
#define VAST_PROFILE_CPU_SCOPE(n)	
#define VAST_PROFILE_CPU_BEGIN(n)	
#define VAST_PROFILE_CPU_END(n)		

#define VAST_PROFILE_GPU_SCOPE(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)
#define VAST_PROFILE_GPU_END(n, ctx)

#define VAST_PROFILE_TRACE_SCOPE(c, n)
#define VAST_PROFILE_TRACE_BEGIN(c, n)
#define VAST_PROFILE_TRACE_END(c, n)
#endif

namespace vast
{
	namespace gfx
	{
		class GraphicsContext;
	}

	namespace Profiler
	{

		void Init(const char* fileName);
		void Stop();

		void EndFrame(gfx::GraphicsContext& ctx);

		void BeginCpuTiming(const char* name);
		void EndCpuTiming();

		void BeginGpuTiming(const char* name, gfx::GraphicsContext& ctx);
		void EndGpuTiming(gfx::GraphicsContext& ctx);

		void BeginTrace(const char* category, const char* name);
		void EndTrace(const char* category, const char* name);

		//

		void OnGUI();

		//

		struct ScopedCpuTiming
		{
			ScopedCpuTiming(const char* name)
			{
				BeginCpuTiming(name);
			}

			~ScopedCpuTiming()
			{
				EndCpuTiming();
			}
		};

		struct ScopedGpuTiming
		{
			ScopedGpuTiming(const char* name, gfx::GraphicsContext& ctx_) : ctx(ctx_)
			{
				BeginGpuTiming(name, ctx);
			}

			~ScopedGpuTiming()
			{
				EndGpuTiming(ctx);
			}

			gfx::GraphicsContext& ctx;
		};

		struct ScopedTrace
		{
			ScopedTrace(const char* category_, const char* name_) : category(category_), name(name_)
			{
				BeginTrace(category, name);
			}

			~ScopedTrace()
			{
				EndTrace(category, name);
			}

			const char* category;
			const char* name;
		};

	}

}