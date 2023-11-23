#include "Color.hlsli"

cbuffer PushConstants : register(PushConstantRegister)
{
    float4x4 ViewMatrix;
    float4x4 ProjMatrix;
    uint EnvironmentTexIdx;
    uint bUseReverseZ;
};

struct VertexOutput
{
    float4 pos	: SV_POSITION;
    float3 uv	: TEXCOORD0;
};

VertexOutput VS_Cube(float3 pos : POSITION)
{
    VertexOutput OUT;
    OUT.pos = mul(ProjMatrix, float4(mul(ViewMatrix, float4(pos, 0.0f)).xyz, 1.0f));
    
    if (bUseReverseZ)
    {
        OUT.pos.z = 0.0f;
    }
    else
    {
        OUT.pos.z = OUT.pos.w;
    }
    
	OUT.uv = pos;
    return OUT;
}

float4 PS_Cube(VertexOutput IN) : SV_TARGET
{
    TextureCube<float4> skyboxTex = ResourceDescriptorHeap[EnvironmentTexIdx];
    SamplerState skyboxSampler = SamplerDescriptorHeap[LinearClampSampler];

    return float4(sRGBtoLinear(skyboxTex.Sample(skyboxSampler, IN.uv).rgb), 1);
}
