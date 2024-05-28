#include "vastpch.h"
#include "Core/Tracing.h"

#include "minitrace/minitrace.h"

namespace vast
{
	namespace Tracing
	{
		void Init(const char* fileName)
		{
#if VAST_ENABLE_TRACING
			mtr_init(fileName);
#endif
		}

		void Stop()
		{
#if VAST_ENABLE_TRACING
			mtr_shutdown();
#endif
		}

		void Flush()
		{
#if VAST_ENABLE_TRACING
			mtr_flush();
#endif
		}

		void BeginTrace(const char* name)
		{
#if VAST_ENABLE_TRACING
			MTR_BEGIN("vast", name);
#endif
		}

		void EndTrace(const char* name)
		{
#if VAST_ENABLE_TRACING
			MTR_END("vast", name);
#endif
		}
	}
}