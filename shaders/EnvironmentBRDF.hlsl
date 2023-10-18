#include "shaders_shared.h"
#include "fullscreen.hlsl"
#include "ImportanceSampling.hlsli"

// Karis 2014 - Real Shading in Unreal Engine 4
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// Derivation based on: https://github.com/google/filament/blob/main/libs/ibl/src/CubemapIBL.cpp
float2 IntegrateBRDF(uint numSamples, float roughness, float NdotV)
{
    // Given an arbitrary N, find V to satisfy NdotV
    float3 N = float3(0, 0, 1);
    float3 V = float3(sqrt(1.0f - NdotV * NdotV), 0, NdotV);

    float2 r = 0;
    for (uint i = 0; i < numSamples; ++i)
    {
        // Sample microfacet direction
        float2 Xi = Hammersley2d(i, numSamples);
        float3 H = HemisphereImportanceSample_GGX(Xi, roughness);        
        float3 L = reflect(-V, H);

        float NdotL = saturate(L.z);        
        if (NdotL > 0)
        {
            float NdotH = saturate(H.z);
            float VdotH = saturate(dot(V, H));

            float Vis = V_SmithGGXHeightCorrelated(NdotV, NdotL, roughness) * NdotL * (VdotH / NdotH);
            float Fc = Pow5(1.0 - VdotH);

#if BSDF_USE_GGX_MULTISCATTER
			r.x += Vis * Fc;
			r.y += Vis;
#else
            r.x += Vis * (1.0f - Fc);
            r.y += Vis * Fc;
#endif
        }
    }

    return r * (4.0f / float(numSamples));
}

float2 PS_IntegrateBRDF(VertexOutputFS IN) : SV_TARGET
{
    float roughness = LinearizeRoughness(IN.uv.x);
    float NdotV = IN.uv.y;
    
    return IntegrateBRDF(1024u, roughness, NdotV);
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
    
    float roughness = LinearizeRoughness(uv.x);
    float NdotV = uv.y;
    
    EnvBrdfUav[threadId.xy] = IntegrateBRDF(NumSamples, roughness, NdotV);
}