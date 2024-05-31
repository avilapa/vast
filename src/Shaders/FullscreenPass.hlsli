#ifndef __FULLSCREEN_PASS_HLSL__
#define __FULLSCREEN_PASS_HLSL__

struct VertexOutputFS
{
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD0;
};

float4 MakeFullscreenTriangle(int vtxId)
{
    // 0 -> pos = [-1, 1], uv = [0,0]
    // 1 -> pos = [-1,-3], uv = [0,2] 
    // 2 -> pos = [ 3, 1], uv = [2,0]
    float4 vtx;
    vtx.zw = float2(vtxId & 2, (vtxId << 1) & 2);
    vtx.xy = vtx.zw * float2(2, -2) + float2(-1, 1);
    return vtx;
}

#endif // __FULLSCREEN_PASS_HLSL__