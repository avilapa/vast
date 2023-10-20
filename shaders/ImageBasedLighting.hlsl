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

// - Irradiance Integration -----------------------------------------------------------------------

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
    return IntegrateDiffuseIrradiance_Discrete(GetSampleDelta(NumSamples), N, p);
#elif DIFFUSE_IRRADIANCE_CONVOLUTION_METHOD == DIFFUSE_IRRADIANCE_CONVOLUTION_MONTE_CARLO
    return IntegrateDiffuseIrradiance_MonteCarlo(NumSamples, N, p);
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
    
    CubemapParams p;
    p.cubeSampler = SamplerDescriptorHeap[LinearClampSampler];
    p.cubeTex = ResourceDescriptorHeap[SkyboxTexIdx];
    p.bConvertToLinear = true;

    IrradianceCubeUav[threadId] = float4(IntegrateDiffuseIrradiance(N, p), 1.0f);
}

// - Radiance Integration -------------------------------------------------------------------------

[numthreads(8, 8, 1)] 
void CS_IntegratePrefilteredRadiance(uint3 threadId : SV_DispatchThreadID)
{   
    RWTexture2DArray<float4> PrefilterCubeUav = ResourceDescriptorHeap[OutputUavTexIdx];
    float3 texSize;
    PrefilterCubeUav.GetDimensions(texSize.x, texSize.y, texSize.z);

    float2 uv = (threadId.xy + 0.5f) / texSize.xy;
    float3 R = normalize(GetCubemapUV(uv, threadId.z));
    float roughness = max(LinearizeRoughness(float(MipLevel) / float(NumMips - 1)), MIN_ROUGHNESS);

    CubemapParams p;
    p.cubeSampler = SamplerDescriptorHeap[LinearClampSampler];
    p.cubeTex = ResourceDescriptorHeap[SkyboxTexIdx];
    p.bConvertToLinear = true;

    PrefilterCubeUav[threadId] = float4(IntegratePrefilteredRadiance(NumSamples, roughness, R, p), 1.0f);
}
