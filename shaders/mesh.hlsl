#include "shaders_shared.h"
#include "Common.hlsli"
#include "BSDF.hlsli"
#include "EnvironmentBRDF.hlsl"
#include "ImageBasedLighting.hlsl"

ConstantBuffer<MeshCB> ObjectConstantBuffer : register(b1, PerObjectSpace);

struct VertexOutput
{
    float4 pos          : SV_POSITION;
    float3 worldPos     : WORLD_POSITION;
    float3 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD0;
};

VertexOutput VS_Main(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
    Vtx3fPos3fNormal2fUv vtx = vtxBuf.Load<Vtx3fPos3fNormal2fUv>(vtxId * sizeof(Vtx3fPos3fNormal2fUv));

	float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.pos, 1)).xyz;
    float3 worldNormal = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.normal, 0)).xyz;

    float3 V = normalize(FrameConstantBuffer.cameraPos - worldPos);
    worldNormal = AdjustBackFacingVertexNormals(worldNormal, V);

	VertexOutput OUT;
	OUT.pos = mul(FrameConstantBuffer.viewProjMatrix, float4(worldPos, 1));
    OUT.worldPos = worldPos;
    OUT.worldNormal = worldNormal;
	OUT.uv = vtx.uv;

    return OUT;
}

float3 SampleIrradiance(float3 N)
{
    SamplerState LinearSampler = SamplerDescriptorHeap[LinearClampSampler];
    TextureCube<float4> IrradianceTexture = ResourceDescriptorHeap[FrameConstantBuffer.ibl.irradianceTexIdx];
    
    return IrradianceTexture.Sample(LinearSampler, N).xyz;
}

float3 SamplePrefilteredRadiance(float3 R, float roughness)
{
    SamplerState LinearSampler = SamplerDescriptorHeap[LinearClampSampler];
    TextureCube<float4> PrefilteredEnvMap = ResourceDescriptorHeap[FrameConstantBuffer.ibl.prefilterTexIdx];
    
    const float mipCount = FrameConstantBuffer.ibl.prefilterNumMips;
    return PrefilteredEnvMap.SampleLevel(LinearSampler, R, roughness * (mipCount - 1)).rgb;
}

float2 SampleEnvBRDF(float roughness, float NdotV)
{
    SamplerState linearSampler = SamplerDescriptorHeap[LinearClampSampler];
    Texture2D<float2> envBrdfLut = ResourceDescriptorHeap[FrameConstantBuffer.ibl.envBrdfLutIdx];
    
    return envBrdfLut.Sample(linearSampler, float2(roughness, NdotV));
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    float3 N = normalize(IN.worldNormal);
    float3 V = normalize(FrameConstantBuffer.cameraPos - IN.worldPos);
    
    float3 baseColor = ObjectConstantBuffer.baseColor;
    if (ObjectConstantBuffer.bUseColorTex)
    {
        SamplerState ColorSampler = SamplerDescriptorHeap[ObjectConstantBuffer.colorSamplerIdx];
        Texture2D<float4> ColorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colorTexIdx];
        
        baseColor *= sRGBtoLinear(ColorTex.Sample(ColorSampler, IN.uv).rgb);
    }
    
    const float dielectricFresnel = 0.04f;
    float metallic = ObjectConstantBuffer.metallic;
    float roughness = max(LinearizeRoughness(ObjectConstantBuffer.roughness), MIN_ROUGHNESS);

    float3 diffuseAlbedo = baseColor * (1.0 - metallic);
    float3 f0 = lerp(dielectricFresnel, baseColor, metallic);

    float3 diffuseIrradiance = SampleIrradiance(N);
    
    float3 R = reflect(-V, N);
    float3 prefilteredRadiance = SamplePrefilteredRadiance(R, ObjectConstantBuffer.roughness);
    if (GetDebugToggle(0))
    {
        CubemapParams p;
        p.cubeSampler = SamplerDescriptorHeap[LinearClampSampler];
        p.cubeTex = ResourceDescriptorHeap[FrameConstantBuffer.skyboxTexIdx];
        p.bConvertToLinear = true;
        
        prefilteredRadiance = IntegratePrefilteredRadiance(1024, roughness, N, V, p);
    }
    
    float NdotV = saturate(dot(N, V));
    float2 envBrdf = SampleEnvBRDF(ObjectConstantBuffer.roughness, NdotV);
    if (GetDebugToggle(1))
    {
        envBrdf = IntegrateEnvBRDF(1024, roughness, NdotV);
    }
        
#if BSDF_USE_GGX_MULTISCATTER
    float3 DFG = lerp(envBrdf.xxx, envBrdf.yyy, f0);
#else
    float3 DFG = f0 * envBrdf.x + envBrdf.y;
#endif 
    
    float3 Fd = diffuseAlbedo * diffuseIrradiance * (1 - DFG);
    float3 Fr = prefilteredRadiance * DFG * GetEnergyCompensationGGX(f0, envBrdf);

    float3 result = Fd + Fr;
    
    return float4(result, 1.0f);
}