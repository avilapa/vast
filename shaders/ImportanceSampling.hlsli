#include "BSDF.hlsl"

// Dammertz 2012 - Hammersley Points on the Hemisphere
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Reverse bits in a 32 bit integer.
float RadicalInverse_VdC(uint i)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley2d(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 Hammersley3d(uint i, uint N)
{
    const float phi = 2 * PI * RadicalInverse_VdC(i);
    return float3(float(i) / float(N), cos(phi), sin(phi));
}

// Note: Sampled in tangent space, with N = (0, 0, 1)
float3 HemisphereImportanceSample_GGX(float2 Xi, float a)
{
    const float phi = 2 * PI * Xi.x;
    // Optimized for better fp accuracy: (a * a - 1) == (a + 1) * (a - 1)
    float cosTheta2 = (1 - Xi.y) / (1 + (a + 1) * ((a - 1) * Xi.y));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1 - cosTheta2);
    
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 HemisphereImportanceSample_GGX(float3 Xi, float a)
{
    // Optimized for better fp accuracy: (a * a - 1) == (a + 1) * (a - 1)
    float cosTheta2 = (1 - Xi.x) / (1 + (a + 1) * ((a - 1) * Xi.x));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1 - cosTheta2);
    
    return float3(sinTheta * Xi.y, sinTheta * Xi.z, cosTheta);
}
