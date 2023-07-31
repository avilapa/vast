#include "vastpch.h"
#include "Core/Profiler.h"

#if VAST_ENABLE_PROFILING
#include "minitrace/minitrace.h"

namespace vast
{

	void Profiler::Init(const char* fileName)
	{
		mtr_init(fileName);
	}

	void Profiler::Stop()
	{
		mtr_flush(); 
		mtr_shutdown();
	}

	void Profiler::BeginTrace(const char* category, const char* name)
	{
		MTR_BEGIN(category, name);
	}

	void Profiler::EndTrace(const char* category, const char* name)
	{
		MTR_END(category, name);
	}

}

#endif
