#pragma once

#include "Core/Defines.h"

#if VAST_ENABLE_PROFILING
#define VAST_PROFILE_SCOPE(c, n)	vast::Profiler::ScopedTrace __profScopedT(c, n)
#define VAST_PROFILE_BEGIN(c, n)	vast::Profiler::BeginTrace(c, n)
#define VAST_PROFILE_END(c, n)		vast::Profiler::EndTrace(c, n)
#else
#define VAST_PROFILE_SCOPE(c, n)
#define VAST_PROFILE_BEGIN(c, n)
#define VAST_PROFILE_END(c, n)
#endif

namespace vast
{

	namespace Profiler
	{

		void Init(const char* fileName);
		void Stop();

		void BeginTrace(const char* category, const char* name);
		void EndTrace(const char* category, const char* name);

		struct ScopedTrace
		{
			ScopedTrace(const char* category, const char* name) : m_Category(category), m_Name(name)
			{
				BeginTrace(m_Category, m_Name);
			}

			~ScopedTrace()
			{
				EndTrace(m_Category, m_Name);
			}

			const char* m_Category;
			const char* m_Name;
		};
	}

}