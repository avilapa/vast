#include "Common.hlsli"

cbuffer PushConstants : register(PushConstantRegister)
{
	uint RenderTargetIdx;
};

VertexOutputFS VS_Main(uint vtxId : SV_VertexID)
{
    float4 vtx = MakeFullscreenTriangle(vtxId);

    VertexOutputFS OUT;
    OUT.pos = float4(vtx.xy, 0, 1);
    OUT.uv = vtx.zw;

    return OUT;
}

float4 PS_Main(VertexOutputFS IN) : SV_TARGET
{
    Texture2D<float4> rt = ResourceDescriptorHeap[RenderTargetIdx];
    SamplerState rtSampler = SamplerDescriptorHeap[PointClampSampler]; 

    float3 color = rt.Sample(rtSampler, IN.uv).rgb;

    return float4(ApplyGammaCorrection(color), 1);
}