#include "shaders_shared.h"

ConstantBuffer<MeshCB> CB : register(b0, PerObjectSpace);

struct VertexOutput
{
    float4 pos          : SV_POSITION;
    float3 worldPos     : WORLD_POSITION;
    float3 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD0;
};

VertexOutput VS_Main(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[CB.vtxBufIdx];
    Vtx3fPos3fNormal2fUv vtx = vtxBuf.Load<Vtx3fPos3fNormal2fUv>(vtxId * sizeof(Vtx3fPos3fNormal2fUv));

    float4x4 viewProj = mul(CB.proj, CB.view);
	float3 worldPos = mul(CB.model, float4(vtx.pos, 1)).xyz;
    float3 worldNormal = mul(CB.model, float4(vtx.normal, 0)).xyz;

	VertexOutput OUT;
	OUT.pos = mul(viewProj, float4(worldPos, 1));
    OUT.worldPos = worldPos;
    OUT.worldNormal = worldNormal;
	OUT.uv = vtx.uv;

    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[CB.colorTexIdx];
    SamplerState colorSampler = SamplerDescriptorHeap[CB.colorSamplerIdx];

    float3 color = colorTex.Sample(colorSampler, IN.uv).rgb;
    float3 lightDirection = normalize(CB.cameraPos);
    float3 viewDirection = normalize(CB.cameraPos - IN.worldPos);

    float3 H = normalize(viewDirection + lightDirection);
    float specular = pow(saturate(dot(IN.worldNormal, H)), 8.0f);
    float diffuse = saturate(dot(IN.worldNormal, lightDirection));

    float3 lighting = color * (diffuse + specular);

    return float4(lighting, 1);
}