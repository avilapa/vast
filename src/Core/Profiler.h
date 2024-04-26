#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_CPU_SCOPE(n)		vast::ProfileScopeCPU XCAT(_profCpu, __LINE__)(n)
#define VAST_PROFILE_CPU_BEGIN(n)		vast::Profiler::PushProfilingMarkerCPU(n)
#define VAST_PROFILE_CPU_END()			vast::Profiler::PopProfilingMarkerCPU()

#define VAST_PROFILE_GPU_SCOPE(n, ctx)	vast::ProfileScopeGPU XCAT(_profGpu, __LINE__)(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)	vast::Profiler::PushProfilingMarkerGPU(n, ctx)
#define VAST_PROFILE_GPU_END()			vast::Profiler::PopProfilingMarkerGPU()
// TODO: Single profile macro, select cpu/gpu
#else
#define VAST_PROFILE_CPU_SCOPE(n)	
#define VAST_PROFILE_CPU_BEGIN(n)	
#define VAST_PROFILE_CPU_END()		

#define VAST_PROFILE_GPU_SCOPE(n, ctx)
#define VAST_PROFILE_GPU_BEGIN(n, ctx)
#define VAST_PROFILE_GPU_END()
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
		static void PushProfilingMarkerCPU(const char* name);
		static void PushProfilingMarkerGPU(const char* name, gfx::GraphicsContext* ctx);
		static void PopProfilingMarkerCPU();
		static void PopProfilingMarkerGPU();

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
		ProfileScopeGPU(const char* name, gfx::GraphicsContext* ctx)
		{
			Profiler::PushProfilingMarkerGPU(name, ctx);
		}

		~ProfileScopeGPU()
		{
			Profiler::PopProfilingMarkerGPU();
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