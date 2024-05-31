#include "WhiteFurnace_shared.h"
#include "Common.hlsli"
#include "Color.hlsli"
#include "BSDF.hlsli"
#include "ImportanceSampling.hlsli"
#include "EnvironmentBRDF.hlsl"

// Heitz 2014 - Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs
// https://jcgt.org/published/0003/02/03/paper.pdf
// The white furnace test is an integration of lighting against a constant white environment where
// the Fresnel term of the BRDF is fixed to 1, meaning there is no refraction so the BTDF evaluates
// to 0. When absorption is 0 all incident radiance will be perfectly preserved during scattering
// and all light rays will be reflected without energy loss or gain, and their distribution is
// normalized.
//
//  Integral(BRDF * NdotL * dL) = 1    =>    (1/N) * Sum(BRDF / PDF * NdotL)  = 1
//
//  BRDF = (D * F * G) / (4 * NdotV * NdotL), F = 1, G = G1(V) * G1(L)
//  PDF = D * NdotH * J, J = 1 / (4 * VdotH)
//
// However common BRDFs only model a single scattering event, meaning rays that reflect again into
// the surface don't reflect again, and instead their energy is discarded, resulting energy loss.
//
// The weak white furnace test by contrast is a less restrictive version of the white furnace test,
// used to validate these single scattering models by removing the G1(L) shadowing term from the
// BRDF evaluation. It verifies that the distribution of rays reflected just after the first bounce
// and before leaving the surface is normalized.
//
//  BRDF = (D * F * G) / (4 * NdotV * NdotL), F = 1, G = G1(V)
//  PDF = D * NdotH * J, J = 1 / (4 * VdotH)
//

ConstantBuffer<WhiteFurnaceConstants> WhiteFurnaceParams : register(PushConstantRegister);

bool IsWhiteFurnaceFlagSet(int bit)
{
    return WhiteFurnaceParams.Flags & bit;
}

float IntegrateWhiteFurnaceTest(uint numSamples, float roughness, float NdotV)
{
    // Given an arbitrary N, find V to satisfy NdotV
    float3 N = float3(0, 0, 1);
    float3 V = float3(sqrt(1.0f - NdotV * NdotV), 0, NdotV);

    float r = 0;
    for (uint i = 0; i < numSamples; ++i)
    {
        // Sample microfacet direction
        float2 Xi = Hammersley2d(i, numSamples);
        float3 H = HemisphereImportanceSample_GGX(Xi, roughness);
        float3 L = reflect(-V, H);

        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));
        
        if (!IsWhiteFurnaceFlagSet(WF_USE_WEAK_WHITE_FURNACE))
        {
            float NdotL = saturate(L.z);
            // Note: This derivation is the same as in IntegrateEnvBRDF (EnvironmentBRDF.hlsl)
#if !BSDF_USE_JOINT_VISIBILITY_FUNCTIONS
            float G2 = G_SmithGGX(NdotV, NdotL, roughness);
            float Vis = G2 * VdotH / (NdotH * NdotV);
#else
        float Vis = V_SmithGGX(NdotV, NdotL, roughness) * NdotL * (4.0f * VdotH / NdotH);
#endif
            r += Vis;
        }
        else
        {
            // Note: We can only use uncorrelated G1 functions since height-correlated counterparts
            // only have a closed form when computed jointly as a G2 function.
            // Note: No point branching on BSDF_USE_JOINT_VISIBILITY_FUNCTIONS here.
            float G1 = G1_GGX(NdotV, roughness);
            float Vis = G1 * VdotH / (NdotH * NdotV);
            r += Vis;
        }
    }
    
    if (IsWhiteFurnaceFlagSet(WF_USE_GGX_MULTISCATTER) && !IsWhiteFurnaceFlagSet(WF_USE_WEAK_WHITE_FURNACE))
    {
        float2 envBrdf = IntegrateEnvBRDF(256, roughness, NdotV);
        r *= GetEnergyCompensationGGX(1, envBrdf).x;
    }

    return r / float(numSamples);
}

[numthreads(32, 32, 1)] 
void CS_WhiteFurnaceTest(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2D<float4> WhiteFurnaceUav = ResourceDescriptorHeap[WhiteFurnaceParams.UavIdx];
    float2 texSize;
    WhiteFurnaceUav.GetDimensions(texSize.x, texSize.y);
    
    float2 uv = (threadId.xy + 0.5f) / texSize;
    
    float roughness = max(LinearizeRoughness(uv.x), MIN_ROUGHNESS);
    float NdotV = uv.y;
    
    float result = IntegrateWhiteFurnaceTest(WhiteFurnaceParams.NumSamples, roughness, NdotV);
    float3 output = result.xxx;
    
    if (IsWhiteFurnaceFlagSet(WF_SHOW_RMSE))
    {
        output = ColormapViridis(RMSE(1.0f, result));
    }

    WhiteFurnaceUav[threadId.xy] = float4(output, 1.0f);
}
