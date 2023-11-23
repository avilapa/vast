#include "shaders_shared.h"
#include "ImportanceSampling.hlsli"

// Karis 2014 - Real Shading in Unreal Engine 4
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// De Rousiers & Lagarde 2014 - Moving Frostbite to PBR, p. 63
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// Guy & Agopian 2018 - Physically Based Rendering in Filament (CubemapIBL.cpp)
// https://github.com/google/filament/blob/main/libs/ibl/src/CubemapIBL.cpp
//
// Note: We assume f90 to be 1, which simplifies F_Schlick to 'Fc + f0 * (1 - Fc)', where
// 'Fc = Pow5(1 - cos(theta))'. This function precalculates the BRDF for when 'f0 = 1' in
// r.x and the case where 'f0 = 0' in r.y, which can be applied as a scale and bias to f0
// when shading a pixel.
float2 IntegrateEnvBRDF(uint numSamples, float roughness, float NdotV)
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
        float3 L = reflect(-V, H); // TODO: Reuse VdotH with a custom reflect function

        float NdotL = saturate(L.z);
        if (NdotL > 0)
        {
            float NdotH = saturate(H.z);
            float VdotH = saturate(dot(V, H));
            
#if 1
            // Derivation for Geometry function:
            // BRDF * (1 / PDF) * NdotL
            // BRDF = (D * F * G) / (4 * NdotV * NdotL), PDF = D * NdotH * J, J = 1 / (4 * VdotH)
            // (D * F * G) / (4 * NdotV * NdotL) * (4 * VdotH) / (D * NdotH) * NdotL
            // NDFs (D) and NdotLs cancel out, giving: 
            // (F * G) / (4 * NdotV) * (4 * VdotH / NdotH) = F * G * VdotH / (NdotH * NdotV)
            float G = G_SmithGGXCorrelated(NdotV, NdotL, roughness);
            float Vis = G * VdotH / (NdotH * NdotV);
#else
            // When instead deriving for a Visibility function (G / BRDF denominator) NdotL's don't
            // cancel out since one is already accounted for. Moreover, the 4 can be taken out of
            // the loop as it is constant.
            float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness) * NdotL * (4.0f * VdotH / NdotH);
#endif
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

    return r / float(numSamples);
}

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