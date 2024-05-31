#ifndef __WHITE_FURNACE_SHARED_HLSL__
#define __WHITE_FURNACE_SHARED_HLSL__

#if defined(__cplusplus)
using namespace vast;
#else
#define uint32 uint
#endif

#define WF_SHOW_RMSE                        1 << 0
#define WF_USE_WEAK_WHITE_FURNACE           1 << 1
#define WF_USE_GGX_MULTISCATTER             1 << 2

struct WhiteFurnaceConstants
{
    uint32 UavIdx;
    uint32 NumSamples;
    uint32 Flags;
};

#endif // __WHITE_FURNACE_SHARED_HLSL__
