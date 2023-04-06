#include "shaders_shared.h"

cbuffer PushConstants : register(PushConstantRegister, PerObjectSpace)
{
    uint32 TriangleVtxBufIdx;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

VertexOutput VS_Main(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[TriangleVtxBufIdx];
    TriangleVtx vtx = vtxBuf.Load<TriangleVtx>(vtxId * sizeof(TriangleVtx));
    
    VertexOutput OUT;
    OUT.pos = float4(vtx.pos, 0, 1);
    OUT.col = vtx.col;
    
    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    return float4(IN.col, 1);
}