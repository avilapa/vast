#include "shaders_shared.h"
#include "fullscreen.hlsl"
#include "ImportanceSampling.hlsli"

cbuffer PushConstants : register(PushConstantRegister)
{
    uint SkyboxTexIdx;
    uint OutputUavTexIdx;
    uint NumSamples;
    uint NumMips;
    uint MipLevel;
};

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

float3 GetEnvironmentSample(float3 uv, bool bIsEnvironmentMapLinear, uint mip = 0)
{
    // TODO: Double check what filtering we're using, as we may need trilinear (mip) for prefilter.
    SamplerState LinearSampler = SamplerDescriptorHeap[LinearClampSampler];
    TextureCube<float4> CubeTex = ResourceDescriptorHeap[SkyboxTexIdx];

    float3 envSample = CubeTex.SampleLevel(LinearSampler, uv, mip).rgb;
    if (!bIsEnvironmentMapLinear)
    {
        envSample = sRGBtoLinear(envSample);
    }
    return envSample;
}

// - Irradiance Integration -----------------------------------------------------------------------

// Coding Labs: Physically based rendering
// http://www.codinglabs.net/article_physically_based_rendering.aspx
// LearnOpenGL: Diffuse Irradiance
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
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

//

#define DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE     0
#define DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO  1

#define DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD       DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE

float GetSampleDelta(const uint numSamples)
{
    return PI / sqrt(float(max(numSamples, 1)));
}

float3 IntegrateDiffuseIrradiance(float3 N, bool bIsEnvironmentMapLinear)
{
#if DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_DISCRETE
    return IntegrateDiffuseIrradiance_Discrete(GetSampleDelta(NumSamples), N, bIsEnvironmentMapLinear);
#elif DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO
    return IntegrateDiffuseIrradiance_MonteCarlo(NumSamples, N, bIsEnvironmentMapLinear);
#endif
}

// Note: Technically this encodes radiance instead of irradiance, since we include the Lambertian
// BRDF for a perfectly white albedo by dividing our result by PI. In practice, this means we don't
// need to divide by PI in the lighting shader when sampling this texture.
[numthreads(32, 32, 1)] 
void CS_IntegrateDiffuseIrradiance(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2DArray<float4> IrradianceCubeUav = ResourceDescriptorHeap[OutputUavTexIdx];
    float3 texSize;
    IrradianceCubeUav.GetDimensions(texSize.x, texSize.y, texSize.z);

    float2 uv = (threadId.xy + 0.5f) / texSize.xy;
    float3 N = normalize(GetCubemapUV(uv, threadId.z));
    
    bool bIsEnvironmentMapLinear = false; // TODO: Support HDR path.
    IrradianceCubeUav[threadId] = float4(IntegrateDiffuseIrradiance(N, bIsEnvironmentMapLinear), 1.0f);
}

// - Environment Convolution ----------------------------------------------------------------------

// Jags 2015 - Image Based Lighting (Aliasing Artifacts)
// https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/
// This method relies on filtering the cubemap by sampling from a lower mip level when the PDF of a
// sample direction is small, the reasoning being that the smaller the PDF, the more texels should
// be averaged for that sample direction. 
//
// In practice, this helps the importance sampling converge much faster.
#define PREFILTER_USE_PDF_FILTERING 1

float3 PrefilterEnvironmentMap(float3 R, float roughness, bool bIsEnvironmentMapLinear)
{
    const float invNumSamples = 1.0f / float(NumSamples);
    float3 N = R;
    float3 V = R;
    
    float3x3 TBN = GetTangentBasis(N);
    
#if PREFILTER_USE_PDF_FILTERING
    TextureCube<float4> SkyboxTex = ResourceDescriptorHeap[SkyboxTexIdx];
    float2 skyboxTexResolution;
    SkyboxTex.GetDimensions(skyboxTexResolution.x, skyboxTexResolution.y);
    // Solid angle subtended by a texel from the environment map at mip level 0.
    const float saTexel = FOUR_PI / (6.0f * skyboxTexResolution.x * skyboxTexResolution.y);
#endif
    
    float3 color = 0;
    float weight = 0;
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 Xi = Hammersley2d(i, NumSamples);
        float3 H = TangentToWorld(HemisphereImportanceSample_GGX(Xi, roughness), TBN);
        float3 L = reflect(-V, H);
        
        float NdotL = dot(N, L);
        if (NdotL > 0)
        {
#if PREFILTER_USE_PDF_FILTERING
            float NdotH = saturate(dot(N, H));
            float D = D_GGX(NdotH, roughness);
            // Note: Cosines cancel out due to the approximation 'N = V' from full formula:
            // 'pdf = D * NdotH / (4.0 * VdotH)'.
            float pdf = D * 0.25f;

            // Solid angle associated with this sample.
            float saSample = 1.0 / (NumSamples * pdf);
            const float mip = max(0.5f * log2(saSample / saTexel) + 1.0f, 0.0f);
#else
            const float mip = 0;
#endif
            color += GetEnvironmentSample(L, bIsEnvironmentMapLinear, mip).rgb * NdotL;
            weight += NdotL;
        }
    }

    return (color / max(weight, 0.1));
}

[numthreads(8, 8, 1)] 
void CS_PrefilterEnvironmentMap(uint3 threadId : SV_DispatchThreadID)
{   
    RWTexture2DArray<float4> PrefilterCubeUav = ResourceDescriptorHeap[OutputUavTexIdx];
    float3 texSize;
    PrefilterCubeUav.GetDimensions(texSize.x, texSize.y, texSize.z);

    float2 uv = (threadId.xy + 0.5f) / texSize.xy;
    float3 R = normalize(GetCubemapUV(uv, threadId.z));
    float roughness = max(LinearizeRoughness(float(MipLevel) / float(NumMips - 1)), MIN_ROUGHNESS);

    bool bIsEnvironmentMapLinear = false; // TODO: Support HDR path.
    PrefilterCubeUav[threadId] = float4(PrefilterEnvironmentMap(R, roughness, bIsEnvironmentMapLinear), 1.0f);
}
