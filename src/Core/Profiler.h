#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_CPU_SCOPE(n)		vast::ProfileScope XCAT(_profCpu, __LINE__)(n)
#define VAST_PROFILE_CPU_BEGIN(n)		vast::Profiler::PushProfilingMarker(n)
#define VAST_PROFILE_CPU_END()			vast::Profiler::PopProfilingMarker()

#define VAST_PROFILE_GPU_SCOPE(n, ctx)	vast::ProfileScope XCAT(_profGpu, __LINE__)(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)	vast::Profiler::PushProfilingMarker(n, ctx)
#define VAST_PROFILE_GPU_END()			vast::Profiler::PopProfilingMarker()
// TODO: Single profile macro, select cpu/gpu

#define VAST_PROFILE_TRACE_SCOPE(c, n)	vast::TraceScope XCAT(_profTrace, __LINE__)(c, n)
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
		void BeginTrace(const char* category, const char* name);
		void EndTrace(const char* category, const char* name);

		void PushProfilingMarker(const char* name, gfx::GraphicsContext* ctx = nullptr);
		void PopProfilingMarker();

		void FlushProfiles();


		void OnGUI();

		// TODO: Turn this into a class with static members?
		// Private members

		void Init(const char* fileName);
		void Stop();

		void UpdateProfiles_Internal();
		void FlushProfiles_Internal();

	}

	struct ProfileScope
	{
		ProfileScope(const char* name, gfx::GraphicsContext* ctx = nullptr)
		{
			Profiler::PushProfilingMarker(name, ctx);
		}

		~ProfileScope()
		{
			Profiler::PopProfilingMarker();
		}
	};

	struct TraceScope
	{
		TraceScope(const char* category_, const char* name_) : category(category_), name(name_)
		{
			Profiler::BeginTrace(category, name);
		}

		~TraceScope()
		{
			Profiler::EndTrace(category, name);
		}

		const char* category;
		const char* name;
	};

}