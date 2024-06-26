#include "vastpch.h"
#include "Core/Tracing.h"

#include "minitrace/minitrace.h"

namespace vast
{
	namespace trace
	{
		void Init(const char* fileName)
		{
			mtr_init(fileName);
		}

		void Stop()
		{
			mtr_flush();
			mtr_shutdown();
		}

		void Flush()
		{
			mtr_flush();
		}

		void BeginTrace(const char* name)
		{
			MTR_BEGIN("vast", name);
		}

		void EndTrace(const char* name)
		{
			MTR_END("vast", name);
		}
	}
}