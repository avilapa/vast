#include "shaders_shared.h"

ConstantBuffer<FullscreenCBV> ObjectConstantBuffer : register(b0, PerObjectSpace);

SamplerState BilinearSampler : register(s0); // TODO: Remove

struct VertexOutput
{
    float4 pos      : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

float4 MakeFullscreenTriangle(int vertexId)
{
    // 0 -> pos = [-1, 1], uv = [0,0]
    // 1 -> pos = [-1,-3], uv = [0,2] 
    // 2 -> pos = [ 3, 1], uv = [2,0]
    float4 OUT;
    OUT.zw = float2(vertexId & 2, (vertexId << 1) & 2);
    OUT.xy = OUT.zw * float2(2, -2) + float2(-1, 1);
    return OUT;
}

VertexOutput VS_Main(uint vertexId : SV_VertexID)
{
    float4 triangleVtx = MakeFullscreenTriangle(vertexId);

    VertexOutput output;
    output.pos = float4(triangleVtx.xy, 0, 1);
    output.uv = triangleVtx.zw;

    return output;
}

float3 sRGBtoLinear(float3 color)
{
    return pow(color, 1.0 / 2.2);
}

float4 PS_Main(VertexOutput input) : SV_TARGET
{
    Texture2D<float4> colorTexture = ResourceDescriptorHeap[ObjectConstantBuffer.texIdx];
    //SamplerState fullscreenSampler = SamplerDescriptorHeap[pointClampSampler]; 

    float3 color = colorTexture.Sample(BilinearSampler, input.uv).rgb; // TODO: Should use a Point Clamp sampler instead

    return float4(sRGBtoLinear(color), 1);
}