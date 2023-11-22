#ifndef __BSDF_HLSL__
#define __BSDF_HLSL__

#define EPS		        0.0001f
#define PI              3.14159265358979323846264338327950288
#define INV_PI          (1.0 / PI)
#define TWO_PI          (2.0 * PI)
#define FOUR_PI         (4.0 * PI)
#define HALF_PI         (0.5 * PI)
#define MIN_ROUGHNESS   0.0002

#define BSDF_USE_GGX_MULTISCATTER 1

float Pow5(float v)
{
    float v2 = v * v;
    return v2 * v2 * v;
}

float LinearizeRoughness(float roughness)
{
    return roughness * roughness;
}

// Walter 2007 - Microfacet Models for Refraction through Rough Surfaces
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
// Karis 2013 - Specular BRDF Reference
// https://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// Guy & Agopian 2018 - Physically Based Rendering in Filament
// https://google.github.io/filament/Filament.html
float D_GGX(float NdotH, float roughness)
{
    // Guy & Agopian 2018 formulation
    float a = NdotH * roughness;
    float k = roughness / (1.0 - NdotH * NdotH + a * a);
    return k * k * INV_PI;
    // Karis 2013 formulation (https://www.desmos.com/calculator/fz0sgowq46)
    // float a2 = roughness * roughness;
    // float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    // return INV_PI * a2 / (t * t);
}

//

float G1_GGX(float NdotV, float roughness)
{
    float a2 = roughness * roughness;
    float NdotV2 = NdotV * NdotV;
    // Walter 2007 formulation
    return 2.0f / (1.0f + sqrt(1.0f + a2 * (1.0f - NdotV2) / NdotV2));
    // Karis 2013 formulation
    // return (2.0f * NdotV) / (NdotV + sqrt(a2 + (1.0f - a2) * NdotV2));
}

float G_SmithGGX(float NdotV, float NdotL, float roughness)
{
    float GGXV = G1_GGX(NdotV, roughness);
    float GGXL = G1_GGX(NdotL, roughness);
    return GGXL * GGXV;
}

// Note: V_SmithGGX = G_SmithGGX / (4.0f * NdotV * NdotL)
float V_SmithGGX(float NdotV, float NdotL, float roughness)
{
    float a2 = roughness * roughness;
    // Karis 2013 formulation
    float GGXV = NdotV + sqrt((NdotV - NdotV * a2) * NdotV + a2);
    float GGXL = NdotL + sqrt((NdotL - NdotL * a2) * NdotL + a2);
    // Burley 2012 formulation
    // float GGXV = NdotV + sqrt(a2 + NdotV2 - a2 * NdotV2);
    // float GGXL = NdotL + sqrt(a2 + NdotL2 - a2 * NdotL2);
    return 1.0f / (GGXL * GGXV);
}

// Guy & Agopian 2018 - Physically Based Rendering in Filament
// https://google.github.io/filament/Filament.html
float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness)
{
    float a2 = roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

// Hammon 2017 - PBR Diffuse Lighting for GGX + Smith Microsurfaces, p. 82
// https://www.gdcvault.com/play/1024478/PBR-Diffuse-Lighting-for-GGX
float V_SmithGGXCorrelatedApprox(float NdotV, float NdotL, float roughness)
{
    float a = roughness;
    float GGXV = NdotL * (NdotV * (1.0 - a) + a);
    float GGXL = NdotV * (NdotL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

//

// Schlick 1994 - An Indexpensive BRDF Model for Physically-based Rendering
// https://wiki.jmonkeyengine.org/docs/3.4/tutorials/_attachments/Schlick94.pdf
float G1_SchlickApprox(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

// Karis 2014 - Real Shading in Unreal Engine 4
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
float K_SchlickGGX(float roughness)
{
    return roughness * 0.5f;
}

float G_SmithSchlickGGX_UE4(float NdotV, float NdotL, float roughness)
{
    float k = K_SchlickGGX(roughness);
    float GGXV = G1_SchlickApprox(NdotV, k);
    float GGXL = G1_SchlickApprox(NdotL, k);
    return GGXL * GGXV;
}

float K_SchlickBeckmann(float roughness)
{
    return roughness * sqrt(2.0f * INV_PI);
}

// TODO: Verify
float G_SmithSchlickBeckmann(float NdotV, float NdotL, float roughness)
{
    float k = K_SchlickBeckmann(roughness);
    float GGXV = G1_SchlickApprox(NdotV, k);
    float GGXL = G1_SchlickApprox(NdotL, k);
    return GGXL * GGXV;
}

//

// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
float3 F_Schlick(float3 f0, float f90, float cosTheta)
{
    return f0 + (f90 - f0) * Pow5(1.0 - cosTheta);
}

// Version optimized for f90 = 1.0
float3 F_Schlick(float3 f0, float cosTheta)
{
    float pow5 = Pow5(1.0 - cosTheta);
    return pow5 + f0 * (1.0 - pow5);
}

float3 GetEnergyCompensationGGX(float3 f0, float2 envBrdf)
{
#if BSDF_USE_GGX_MULTISCATTER
    return 1.0.xxx + f0 * (1.0 / envBrdf.y - 1.0);
#else
    return 1.0.xxx;
#endif
}

#endif // __BSDF_HLSL__