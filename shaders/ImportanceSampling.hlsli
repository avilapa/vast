#ifndef __IMPORTANCE_SAMPLING_HLSL__
#define __IMPORTANCE_SAMPLING_HLSL__

#include "Common.hlsli"
#include "BSDF.hlsli"

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
    const float phi = 2 * PI * Xi.y;
    // Optimized for better fp accuracy: (a * a - 1) == (a + 1) * (a - 1)
    float cosTheta2 = (1 - Xi.x) / (1 + (a + 1) * ((a - 1) * Xi.x));
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

float SphereSampleUniformPDF()
{
    return 1.0 / (4.0 * PI);
}

float HemisphereSampleUniformPDF()
{
    return 1.0 / (2.0 * PI);
}

float HemisphereSampleCosinePDF(float cosTheta)
{
    return cosTheta * (1.0 / PI);
}

float3 HemisphereSampleUniform(float2 Xi)
{
    const float phi = 2 * PI * Xi.y;
    float cosTheta = Xi.x;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 HemisphereSampleCosine(float2 Xi)
{
    const float phi = 2 * PI * Xi.y;
    float cosTheta = sqrt(Xi.x);
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
        
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

//

// Coding Labs: Physically based rendering
// http://www.codinglabs.net/article_physically_based_rendering.aspx
// LearnOpenGL: Diffuse Irradiance
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
float3 IntegrateDiffuseIrradiance_Discrete(float sampleDelta, float3 N, CubemapParams p)
{
    const float invNumSamples = 1.0f / float(uint(TWO_PI / sampleDelta) * uint(HALF_PI / sampleDelta));
    float3x3 TBN = GetTangentBasis(N);
    
    float3 irradiance = 0;
    for (float phi = 0.0; phi < TWO_PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < HALF_PI; theta += sampleDelta)
        {
            float cosTheta = cos(theta);
            float sinTheta = sin(theta); // = sqrt(1.0 - cosTheta * cosTheta)

            float3 L = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
            float NdotL = saturate(L.z);
            
            // Note: We multiply by sin(theta) to compensate the distrubtion of sampled points as 
            // when using spherical coordinates we get more samples at the top of the hemisphere.
            // 'dw_i = sin(theta) dtheta dphi'
            irradiance += SampleCubemap(p, TangentToWorld(L, TBN)) * NdotL * sinTheta;
        }
    }

    // Note: This PI comes from applying a Monte Carlo estimator to each integral on the convolution
    // ('TWO_PI / N1' and 'HALF_PI / N2'), which when scaled by the Lambertian reduces to PI / N1N2.
    return irradiance * PI * invNumSamples;
}

// Monte Carlo integration '(1 / N) * Sum[ f(x) / PDF ]' of hemispherical irradiance.
float3 IntegrateDiffuseIrradiance_MonteCarlo(uint numSamples, float3 N, CubemapParams p)
{
    const float invNumSamples = 1.0f / float(numSamples);
    float3x3 TBN = GetTangentBasis(N);

    float3 irradiance = 0;
    for (uint i = 0; i < numSamples; ++i)
    {
        float2 Xi = Hammersley2d(i, numSamples);
        float3 L = HemisphereSampleUniform(Xi);
        float NdotL = saturate(L.z); // = saturate(dot(N, TangentToWorld(L, TBN))

        irradiance += SampleCubemap(p, TangentToWorld(L, TBN)) * NdotL;
    }
   
    // Note: PI from Lambertian cancels out with division by PDF 
    // 'Li * (1 / PI) * (1 / 2 * PI)  => Li * 2'
    // float pdf = HemisphereSampleUniformPDF();
    // return irradiance * invNumSamples * INV_PI / pdf;
    return irradiance * invNumSamples * 2.0f;
}

//

// GPU-Based Importance Sampling (GPU Gems 3, Chapter 20)
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
// Jags 2015 - Image Based Lighting (Aliasing Artifacts)
// https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/
// This method relies on filtering the cubemap by sampling from a lower mip level when the PDF of a
// sample direction is small, the reasoning being that the smaller the PDF, the more texels should
// be averaged for that sample direction. 
//
// In practice, this helps the importance sampling converge much faster.
#define RADIANCE_USE_PDF_FILTERING      0

float3 IntegratePrefilteredRadiance(uint numSamples, float roughness, float3 N, float3 V, CubemapParams p, bool bUsingNeqVApproximation = false)
{
    const float invNumSamples = 1.0f / float(numSamples);
    float3x3 TBN = GetTangentBasis(N);

#ifdef RADIANCE_USE_PDF_FILTERING
    float2 cubeTexResolution;
    p.cubeTex.GetDimensions(cubeTexResolution.x, cubeTexResolution.y);
    // Solid angle subtended by a texel from the environment map at mip level 0.
    const float saTexel = FOUR_PI / (6.0f * cubeTexResolution.x * cubeTexResolution.y);
#endif
    
    float3 radiance = 0;
    float weight = 0;
    for (uint i = 0; i < numSamples; ++i)
    {
        float2 Xi = Hammersley2d(i, numSamples);
        float3 H = TangentToWorld(HemisphereImportanceSample_GGX(Xi, roughness), TBN);
        float3 L = reflect(-V, H);
        
        float NdotL = dot(N, L);
        if (NdotL > 0)
        {
#ifdef RADIANCE_USE_PDF_FILTERING
            float NdotH = saturate(dot(N, H));
            float D = D_GGX(NdotH, roughness);
            float pdf;
            if (bUsingNeqVApproximation)
            {
                // Note: Cosines cancel out when using the approximation 'N = V'.
                pdf = D * 0.25f;
            }
            else
            {
                float VdotH = saturate(dot(V, H));
                pdf = D * NdotH / (VdotH * 4.0f);
            }

            // Solid angle associated with this sample.
            float saSample = invNumSamples / max(pdf, EPS);
            // TODO: Double check what filtering we're using, as we may need trilinear (mip) for prefilter.
            const float mip = max(0.5f * log2(saSample / saTexel) + 1.0f, 0.0f);
#else
            const float mip = 0;
#endif
            float3 envSample = SampleCubemap(p, L, mip);
            
            radiance += envSample * NdotL;
            weight += NdotL;
        }
    }

    return (radiance / weight);
}

float3 IntegratePrefilteredRadiance(uint numSamples, float roughness, float3 R, CubemapParams p)
{
    // Note: When integrating the non constant part of the split-sum, we make the assumption that
    // N = V = R, which leads to a complete loss of roughness anisotropy with respect to the view
    // point.
    bool bUsingNeqVApproximation = true;
    return IntegratePrefilteredRadiance(numSamples, roughness, R, R, p, bUsingNeqVApproximation);
}

//

// Karis 2014 - Real Shading in Unreal Engine 4
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// Derivation based on: https://github.com/google/filament/blob/main/libs/ibl/src/CubemapIBL.cpp
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

#endif // __IMPORTANCE_SAMPLING_HLSL__
