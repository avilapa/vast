#include "PerFrame_shared.h"
#include "VertexInputs_shared.h"
#include "Common.hlsli"
#include "Color.hlsli"
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

// Moving Frostbite to PBR
float3 GetSpecularDominantDir(float3 N, float3 R, float roughness)
{
    float a = roughness * roughness;
    return lerp(N, R, (1 - a) * (sqrt(1 - a) + a));
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

float IORtoF0(float eta_i, float eta_t)
{
    float e = saturate((eta_t - eta_i) / (eta_t + eta_i));
    return e * e;
}

float3 ThinFilmEvalSensitivity(float opd, float shift)
{
	// Use Gaussian fits, given by 3 parameters: val, pos and var
    float phase = 2 * PI * opd * 1.0e-6;
    float3 val = float3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    float3 pos = float3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    float3 var = float3(4.3278e+09, 9.3046e+09, 6.6121e+09);
    float3 xyz = val * sqrt(2 * PI * var) * cos(pos * phase + shift) * exp(-var * phase * phase);
    xyz.x += 9.7470e-14 * sqrt(2 * PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift) * exp(-4.5282e+09 * phase * phase);
    return xyz / 1.0685e-7;
	// As pointed by the suplemental material the oupout is not in the correct space, we need to tranfser 
}

float3 F_ThinFilm(float ct1, float3 f0, float eta2, float Dinc)
{
    eta2 = lerp(1.0, eta2, smoothstep(0.0, 0.03, Dinc));
    float ct2 = sqrt(1.0 - Pow2(1.0 / eta2) * (1 - Pow2(ct1)));

	// First interface is dieletric so it is achromatic
    float filmF0 = IORtoF0(1.0, eta2);
    float filmF90 = saturate(50.0f * dot(filmF0, 0.33));
    float R12 = F_Schlick(filmF0, filmF90, ct1).x;
    float phi12 = 0;
    float R21 = R12;
    float T121 = 1 - R12;
    float phi21 = PI - phi12;

		// Second interface is either conductor or dieletric, so can be chromatic. 
		// Ideally we should recompute F0 (R23) by taking into account the thin layer IOR
    float3 R23 = f0;
    float phi23 = 0;

		// Phase shift
    float OPD = Dinc * ct2;
    float phi2 = phi21 + phi23;

		// Compound terms
    float3 R123 = R12 * R23;
    float3 r123 = sqrt(R123);
    float3 Rs = Pow2(T121) * R23 / (1 - R123);

		// Reflectance term for m=0 (DC term amplitude)
    float3 C0 = R12 + Rs;
    float3 S0 = ThinFilmEvalSensitivity(0.0, 0.0);
    float3 I = C0 * S0;

		// Reflectance term for m>0 (pairs of diracs)
		// Simplified based as we don't take into account polarization
    float3 Cm = Rs - T121;
    for (int m = 1; m <= 3; ++m)
    {
        Cm *= r123;
        I += Cm * 2.0 * ThinFilmEvalSensitivity(m * OPD, m * phi2);
    }

		// Convert back to RGB reflectance
		// XYZ to CIE 1931 RGB color space (using neutral E illuminant)
    const float3x3 ThinFilm_XYZ_TO_RGB = float3x3(2.3706743, -0.5138850, 0.0052982, -0.9000405, 1.4253036, -0.0146949, -0.4706338, 0.0885814, 1.0093968);
    return saturate(mul(I, ThinFilm_XYZ_TO_RGB));
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
    
    float NdotV = saturate(dot(N, V));
    
    {
        SamplerState NoiseSampler = SamplerDescriptorHeap[LinearClampSampler];
        Texture2D<float4> NoiseTex = ResourceDescriptorHeap[ObjectConstantBuffer.noiseTexIdx];
        
        float Dinc = NoiseTex.Sample(NoiseSampler, IN.uv).x * 3.0f;
        
        f0 = F_ThinFilm(NdotV, f0, 2.5, Dinc);
    }
    
    float3 diffuseIrradiance = SampleIrradiance(N);
    
    float3 R = reflect(-V, N);
    float3 prefilteredRadiance = SamplePrefilteredRadiance(GetDebugToggle(5) ? GetSpecularDominantDir(N, R, roughness) : R, ObjectConstantBuffer.roughness);
    if (GetDebugToggle(0))
    {
        CubemapParams p;
        p.cubeSampler = SamplerDescriptorHeap[LinearClampSampler];
        p.cubeTex = ResourceDescriptorHeap[FrameConstantBuffer.skyboxTexIdx];
        p.bConvertToLinear = true;
        
        prefilteredRadiance = IntegratePrefilteredRadiance(256, roughness, N, V, p);
    }
    
    float2 envBrdf = SampleEnvBRDF(ObjectConstantBuffer.roughness, NdotV);
    if (GetDebugToggle(1))
    {
        envBrdf = IntegrateEnvBRDF(256, roughness, NdotV);
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