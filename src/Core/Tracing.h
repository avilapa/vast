#pragma once

#if VAST_ENABLE_TRACING
#define VAST_PROFILE_TRACE_FUNCTION	::vast::trace::ScopedTrace XCAT(_traceVar, __LINE__)(__FUNCTION__)
#define VAST_PROFILE_TRACE_SCOPE(n)	::vast::trace::ScopedTrace XCAT(_traceVar, __LINE__)(n)
#define VAST_PROFILE_TRACE_BEGIN(n)	::vast::trace::BeginTrace(n)
#define VAST_PROFILE_TRACE_END(n)	::vast::trace::EndTrace(n)
#else
#define VAST_PROFILE_TRACE_FUNCTION
#define VAST_PROFILE_TRACE_SCOPE(n)
#define VAST_PROFILE_TRACE_BEGIN(n)
#define VAST_PROFILE_TRACE_END(n)
#endif

namespace vast
{
	namespace trace
	{
		void Init(const char* fileName);
		void Stop();
		void Flush();

		void BeginTrace(const char* funcName);
		void EndTrace(const char* funcName);

		struct ScopedTrace
		{
			ScopedTrace(const char* funcName) : m_FuncName(funcName) { BeginTrace(m_FuncName); }
			~ScopedTrace() { EndTrace(m_FuncName); }

		private:
			const char* m_FuncName;
		};
	}
}
