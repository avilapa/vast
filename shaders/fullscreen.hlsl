#include "shaders_shared.h"

cbuffer PushConstants : register(PushConstantRegister, PerObjectSpace)
{
	uint32 ColorTexIdx;
};

SamplerState BilinearSampler : register(s0); // TODO: Remove

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
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

VertexOutput VS_Main(uint vtxId : SV_VertexID)
{
    float4 vtx = MakeFullscreenTriangle(vtxId);

    VertexOutput OUT;
    OUT.pos = float4(vtx.xy, 0, 1);
    OUT.uv = vtx.zw;

    return OUT;
}

float3 sRGBtoLinear(float3 col)
{
    return pow(col, 1.0 / 2.2);
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ColorTexIdx];
    //SamplerState fullscreenSampler = SamplerDescriptorHeap[pointClampSampler]; 

    float3 color = colorTex.Sample(BilinearSampler, IN.uv).rgb; // TODO: Should use a Point Clamp sampler instead

    return float4(sRGBtoLinear(color), 1);
}