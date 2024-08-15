#pragma once

// - Platform ---------------------------------------------------------------------------------- //

#ifdef _WIN32
#ifdef _WIN64
#define VAST_PLATFORM_WINDOWS
#else
#error "Invalid Platform: x86 builds not supported"
#endif
#else
#error "Invalid Platform: Unknown Platform"
#endif

// - Assert ------------------------------------------------------------------------------------ //

#ifdef VAST_DEBUG
#define VAST_ENABLE_ASSERTS 1
#define VAST_DEBUGBREAK() __debugbreak()
#else
#define VAST_ENABLE_ASSERTS 0
#define VAST_DEBUGBREAK()
#endif

// - Logging ----------------------------------------------------------------------------------- //

#define VAST_ENABLE_LOGGING 1

#if VAST_ENABLE_LOGGING
#define VAST_LOGGING_ONLY(x) x
#else
#define VAST_LOGGING_ONLY(x)
#endif

#define VAST_ENABLE_LOGGING_RESOURCE_BARRIERS (0 && VAST_ENABLE_LOGGING)

// - Profiling --------------------------------------------------------------------------------- //

#define VAST_ENABLE_PROFILING 1

#if VAST_ENABLE_PROFILING
#define VAST_PROFILING_ONLY(x) x
#else
#define VAST_PROFILING_ONLY(x)
#endif

#define VAST_ENABLE_TRACING	1

#if VAST_ENABLE_TRACING
#define VAST_TRACING_ONLY(x) x
#else
#define VAST_TRACING_ONLY(x)
#endif


// - Graphics ---------------------------------------------------------------------------------- //

#ifdef VAST_PLATFORM_WINDOWS
#define VAST_GFX_DX12_SUPPORTED 1
#else
#define VAST_GFX_DX12_SUPPORTED 0
#endif

#define VAST_GFX_DEPTH_DEFAULT_USE_REVERSE_Z 1

// - Utility ----------------------------------------------------------------------------------- //

// Macro expansion
#define EXP(x)		x
// Macro stringify
#define STR(x)		#x
#define XSTR(x)		STR(x)
// Macro concatenation
#define CAT(a, b)	a ## b
#define XCAT(a, b)	CAT(a, b)

#define SELECT_MACRO(_1, _2, x, ...) x
// Usage: #define FOO(...) EXP(SELECT_MACRO(__VA_ARGS__, FOO_2_PARAMS, FOO_1_PARAM)(__VA_ARGS__))

// Enum as index
#define IDX(x) static_cast<int>(x)

// Number of elements in array (pointers not supported)
#define NELEM(x) int(sizeof(x) / sizeof((x)[0]))

#define BIT(x) (1 << x)

// Compile time count bits for powers of two
constexpr unsigned int CountBits(unsigned int x) { return (x < 2) ? x : 1 + CountBits(x >> 1); }

#define PI 3.14159265358979323846264338327950288

#define DEG_TO_RAD(x) (x * float(PI/180.0))
#define RAD_TO_DEG(x) (x * float(180.0/PI))
