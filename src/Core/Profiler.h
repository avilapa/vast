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
#else
#define VAST_PROFILE_CPU_SCOPE(n)	
#define VAST_PROFILE_CPU_BEGIN(n)	
#define VAST_PROFILE_CPU_END(n)		

#define VAST_PROFILE_GPU_SCOPE(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)
#define VAST_PROFILE_GPU_END(n, ctx)
#endif

#if VAST_ENABLE_TRACING
#define VAST_PROFILE_TRACE_SCOPE(c, n)	vast::TraceScope XCAT(_profTrace, __LINE__)(c, n)
#define VAST_PROFILE_TRACE_BEGIN(c, n)	vast::TraceLogger::BeginTrace(c, n)
#define VAST_PROFILE_TRACE_END(c, n)	vast::TraceLogger::EndTrace(c, n)
#else
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

	class Profiler
	{
		friend class WindowedApp;
	public:
		static void PushProfilingMarker(const char* name, gfx::GraphicsContext* ctx = nullptr);
		static void PopProfilingMarker();

		static void FlushProfiles();

		static void OnGUI();
	private:
		static void Init();

		static void BeginFrame();
		static void EndFrame(gfx::GraphicsContext& ctx);
	};

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

	class TraceLogger
	{
		friend class WindowedApp;
	public:
		static void BeginTrace(const char* category, const char* name);
		static void EndTrace(const char* category, const char* name);

	private:
		static void Init(const char* fileName);
		static void Stop();

	};

	struct TraceScope
	{
		TraceScope(const char* category_, const char* name_) : category(category_), name(name_)
		{
			TraceLogger::BeginTrace(category, name);
		}

		~TraceScope()
		{
			TraceLogger::EndTrace(category, name);
		}

		const char* category;
		const char* name;
	};

}