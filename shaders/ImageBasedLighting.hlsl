#include "shaders_shared.h"
#include "Common.hlsli"
#include "BSDF.hlsli"
#include "ImportanceSampling.hlsli"

struct IBLConstants
{
    uint SkyboxTexIdx;
    uint OutputUavTexIdx;
    uint NumSamples;
    uint NumMips;
    uint MipLevel;
};
ConstantBuffer<IBLConstants> IBLParams : register(PushConstantRegister);

float3 GetCubemapUV(float2 uv, uint cubeFace)
{
    uv = float2(uv.x, 1 - uv.y) * 2.0f - 1.0f;

    switch (cubeFace)
    {
	    case 0: return normalize(float3( 1.0f,  uv.y, -uv.x));
	    case 1: return normalize(float3(-1.0f,  uv.y,  uv.x));
	    case 2: return normalize(float3( uv.x,  1.0f, -uv.y)); 
	    case 3: return normalize(float3( uv.x, -1.0f,  uv.y)); 
	    case 4: return normalize(float3( uv.x,  uv.y,  1.0f));
	    case 5: return normalize(float3(-uv.x,  uv.y, -1.0f));
        default: return 0;
    }
}

// - Irradiance Integration -----------------------------------------------------------------------

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

#define DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE     0
#define DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO  1

#define DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD       DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE

float GetSampleDelta(const uint numSamples)
{
    return PI / sqrt(float(max(numSamples, 1)));
}

float3 IntegrateDiffuseIrradiance(float3 N, CubemapParams p)
{
#if DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE
    return IntegrateDiffuseIrradiance_Discrete(GetSampleDelta(IBLParams.NumSamples), N, p);
#elif DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO
    return IntegrateDiffuseIrradiance_MonteCarlo(IBLParams.NumSamples, N, p);
#endif
}

// Note: Technically this encodes radiance instead of irradiance, since we include the Lambertian
// BRDF for a perfectly white albedo by dividing our result by PI. In practice, this means we don't
// need to divide by PI in the lighting shader when sampling this texture.
[numthreads(32, 32, 1)] 
void CS_IntegrateDiffuseIrradiance(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2DArray<float4> IrradianceCubeUav = ResourceDescriptorHeap[IBLParams.OutputUavTexIdx];
    float3 texSize;
    IrradianceCubeUav.GetDimensions(texSize.x, texSize.y, texSize.z);

    float2 uv = (threadId.xy + 0.5f) / texSize.xy;
    float3 N = normalize(GetCubemapUV(uv, threadId.z));
    
    CubemapParams p;
    p.cubeSampler = SamplerDescriptorHeap[LinearClampSampler];
    p.cubeTex = ResourceDescriptorHeap[IBLParams.SkyboxTexIdx];
    p.bConvertToLinear = true;

    IrradianceCubeUav[threadId] = float4(IntegrateDiffuseIrradiance(N, p), 1.0f);
}

// - Radiance Integration -------------------------------------------------------------------------

// GPU-Based Importance Sampling (GPU Gems 3, Chapter 20)
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
// Jags 2015 - Image Based Lighting (Aliasing Artifacts)
// https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/
// This method relies on filtering the cubemap by sampling from a lower mip level when the PDF of a
// sample direction is small, the reasoning being that the smaller the PDF, the more texels should
// be averaged for that sample direction. 
//
// In practice, this helps the importance sampling converge much faster.
#define RADIANCE_USE_PDF_FILTERING      1

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
                // Note: VdotH comes from the Jacobian of the reflection transformation.
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

    return (radiance / max(weight, EPS));
}

float3 IntegratePrefilteredRadiance(uint numSamples, float roughness, float3 R, CubemapParams p)
{
    // Note: When integrating the non constant part of the split-sum, we make the assumption that
    // N = V = R, which leads to a complete loss of roughness anisotropy with respect to the view
    // point.
    bool bUsingNeqVApproximation = true;
    return IntegratePrefilteredRadiance(numSamples, roughness, R, R, p, bUsingNeqVApproximation);
}

[numthreads(8, 8, 1)] 
void CS_IntegratePrefilteredRadiance(uint3 threadId : SV_DispatchThreadID)
{   
    RWTexture2DArray<float4> PrefilterCubeUav = ResourceDescriptorHeap[IBLParams.OutputUavTexIdx];
    float3 texSize;
    PrefilterCubeUav.GetDimensions(texSize.x, texSize.y, texSize.z);

    float2 uv = (threadId.xy + 0.5f) / texSize.xy;
    float3 R = normalize(GetCubemapUV(uv, threadId.z));
    float roughness = max(LinearizeRoughness(float(IBLParams.MipLevel) / float(IBLParams.NumMips - 1)), MIN_ROUGHNESS);

    CubemapParams p;
    p.cubeSampler = SamplerDescriptorHeap[LinearClampSampler];
    p.cubeTex = ResourceDescriptorHeap[IBLParams.SkyboxTexIdx];
    p.bConvertToLinear = true;

    PrefilterCubeUav[threadId] = float4(IntegratePrefilteredRadiance(IBLParams.NumSamples, roughness, R, p), 1.0f);
}
