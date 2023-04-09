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
    MeshVtx vtx = vtxBuf.Load<MeshVtx>(vtxId * sizeof(MeshVtx));

    VertexOutput OUT;
    OUT.pos = mul(CB.model, float4(vtx.pos, 1));
    OUT.worldPos = OUT.pos.xyz;
    OUT.pos = mul(CB.view, OUT.pos);
    OUT.pos = mul(CB.proj, OUT.pos);
    OUT.uv = vtx.uv;
    OUT.worldNormal = mul(CB.model, float4(vtx.normal, 0)).xyz;

    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[CB.colorTexIdx];
    SamplerState colorSampler = SamplerDescriptorHeap[PointClampSampler];

    float3 color = colorTex.Sample(anisoSampler, IN.uv).rgb;
    //float3 color = IN.worldNormal * 0.5 + 0.5;
    float3 lightDirection = normalize(CB.cameraPos);
    float3 viewDirection = normalize(CB.cameraPos - IN.worldPos);

    float3 H = normalize(viewDirection + lightDirection);
    float specular = pow(saturate(dot(IN.worldNormal, H)), 8.0f);
    float diffuse = saturate(dot(IN.worldNormal, lightDirection));

    float3 lighting = color * (diffuse + specular);

    return float4(lighting, 1);
}