#include "shaders_shared.h"
#include "fullscreen.hlsl"
#include "ImportanceSampling.hlsli"

float2 PS_IntegrateBRDF(VertexOutputFS IN) : SV_TARGET
{
    float roughness = max(LinearizeRoughness(IN.uv.x), MIN_ROUGHNESS);
    float NdotV = IN.uv.y;
    
    return IntegrateEnvBRDF(1024u, roughness, NdotV);
}

cbuffer PushConstants : register(PushConstantRegister)
{
    uint EnvBrdfUavIdx;
    uint NumSamples;
};

[numthreads(32, 32, 1)] 
void CS_IntegrateBRDF(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2D<float2> EnvBrdfUav = ResourceDescriptorHeap[EnvBrdfUavIdx];
    float2 texSize;
    EnvBrdfUav.GetDimensions(texSize.x, texSize.y);
    
    float2 uv = (threadId.xy + 0.5f) / texSize;
    
    float roughness = max(LinearizeRoughness(uv.x), MIN_ROUGHNESS);
    float NdotV = uv.y;
    
    EnvBrdfUav[threadId.xy] = IntegrateEnvBRDF(NumSamples, roughness, NdotV);
}