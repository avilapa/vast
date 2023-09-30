#include "shaders_shared.h"
#include "fullscreen.hlsl"
#include "ImportanceSampling.hlsli"

#define DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE     0
#define DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO  1

#define DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD       DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE

cbuffer PushConstants : register(PushConstantRegister)
{
    uint CubeTexIdx;
    uint IrradianceCubeUavIdx;
};

float3 GetSamplingVector(uint3 threadId)
{
    float2 uv = (threadId.xy + 0.5f) / float2(BRDF_IRRADIANCE_TEX_RES.xx);
    uv = float2(uv.x, 1 - uv.y) * 2.0f - 1.0f;

    switch (threadId.z)
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

float3 GetEnvironmentSample(float3 uv, bool bIsEnvironmentMapLinear)
{
    SamplerState LinearSampler = SamplerDescriptorHeap[LinearClampSampler];
    TextureCube<float4> CubeTex = ResourceDescriptorHeap[CubeTexIdx];

    float3 envSample = CubeTex.SampleLevel(LinearSampler, uv, 0).rgb;
    if (!bIsEnvironmentMapLinear)
    {
        envSample = sRGBtoLinear(envSample);
    }
    return envSample;
}

float GetSampleDelta(const uint numSamples)
{
    return PI / sqrt(float(max(numSamples, 1)));
}

// Coding Labs: Physically based rendering
// http://www.codinglabs.net/article_physically_based_rendering.aspx
float3 IntegrateDiffuseIrradiance_Discrete(float sampleDelta, float3 N, bool bIsEnvironmentMapLinear)
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
            irradiance += GetEnvironmentSample(TangentToWorld(L, TBN), bIsEnvironmentMapLinear) * NdotL * sinTheta;
        }
    }

    // Note: This PI comes from applying a Monte Carlo estimator to each integral on the convolution
    // ('TWO_PI / N1' and 'HALF_PI / N2'), which when scaled by the Lambertian reduces to PI / N1N2.
    return irradiance * PI * invNumSamples;
}

// Monte Carlo integration '(1 / N) * Sum[ f(x) / PDF ]' of hemispherical irradiance.
float3 IntegrateDiffuseIrradiance_MonteCarlo(uint numSamples, float3 N, bool bIsEnvironmentMapLinear)
{
    const float invNumSamples = 1.0f / float(numSamples);
    float3x3 TBN = GetTangentBasis(N);

    float3 irradiance = 0;
    for (uint i = 0; i < numSamples; ++i)
    {
        float2 Xi = Hammersley2d(i, numSamples);
        float3 L = HemisphereSampleUniform(Xi);
        float NdotL = saturate(L.z); // = saturate(dot(N, TangentToWorld(L, TBN))

        irradiance += GetEnvironmentSample(TangentToWorld(L, TBN), bIsEnvironmentMapLinear) * NdotL;
    }
   
    // Note: PI cancels out with division by PDF: Li * (1 / PI) * (1 / 2 * PI)  => Li * 2
    // float pdf = HemisphereSampleUniformPDF();
    // return irradiance * invNumSamples * INV_PI / pdf;
    return irradiance * invNumSamples * 2.0f;
}

float3 IntegrateDiffuseIrradiance(uint numSamples, float3 N, bool bIsEnvironmentMapLinear)
{
#if DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE
    return IntegrateDiffuseIrradiance_Discrete(GetSampleDelta(numSamples), N, bIsEnvironmentMapLinear);
#elif DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO
    return IntegrateDiffuseIrradiance_MonteCarlo(numSamples, N, bIsEnvironmentMapLinear);
#endif
}

// Note: Technically this encodes radiance instead of irradiance, since we include the Lambertian
// BRDF for a perfectly white albedo by dividing our result by PI. In practice, this means we don't
// need to divide by PI in the lighting shader when sampling this texture.
[numthreads(32, 32, 1)] 
void CS_IntegrateDiffuseIrradiance(uint3 threadId : SV_DispatchThreadID)
{
    bool bIsEnvironmentMapLinear = false; // TODO: Support HDR path.
    float3 N = normalize(GetSamplingVector(threadId));
    
    RWTexture2DArray<float4> IrradianceCubeUav = ResourceDescriptorHeap[IrradianceCubeUavIdx];
    IrradianceCubeUav[threadId] = float4(IntegrateDiffuseIrradiance(16384u, N, bIsEnvironmentMapLinear), 1.0f);
}
