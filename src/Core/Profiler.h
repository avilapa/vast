#pragma once

#include "Core/Core.h"

// Inline Profiler

#if VAST_ENABLE_PROFILING
#include "minitrace/minitrace.h"

#define VAST_PROFILE_INIT(fileName)	mtr_init(fileName);
#define VAST_PROFILE_STOP()			mtr_flush(); mtr_shutdown();

#define VAST_PROFILE_FUNCTION()		MTR_SCOPE_FUNC()
#define VAST_PROFILE_SCOPE(c, n)	MTR_SCOPE(c, n)
#define VAST_PROFILE_BEGIN(c, n)	MTR_BEGIN(c, n)
#define VAST_PROFILE_END(c, n)		MTR_END(c, n)
#else
#define VAST_PROFILE_INIT(fileName)
#define VAST_PROFILE_STOP()

#define VAST_PROFILE_FUNCTION()
#define VAST_PROFILE_SCOPE(c, n)
#define VAST_PROFILE_BEGIN(c, n)
#define VAST_PROFILE_END(c, n)
#endif
