#include "shaders_shared.h"

ConstantBuffer<SimpleRenderer_PerFrame> FrameConstantBuffer : register(b0, PerObjectSpace);
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

	float3 worldPos = mul(ObjectConstantBuffer.model, float4(vtx.pos, 1)).xyz;
    float3 worldNormal = mul(ObjectConstantBuffer.model, float4(vtx.normal, 0)).xyz;

	VertexOutput OUT;
	OUT.pos = mul(FrameConstantBuffer.viewProjMatrix, float4(worldPos, 1));
    OUT.worldPos = worldPos;
    OUT.worldNormal = worldNormal;
	OUT.uv = vtx.uv;

    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colorTexIdx];
    SamplerState colorSampler = SamplerDescriptorHeap[ObjectConstantBuffer.colorSamplerIdx];

    float3 color = colorTex.Sample(colorSampler, IN.uv).rgb;
    float3 lightDirection = normalize(FrameConstantBuffer.cameraPos);
    float3 viewDirection = normalize(FrameConstantBuffer.cameraPos - IN.worldPos);

    float3 H = normalize(viewDirection + lightDirection);
    float specular = pow(saturate(dot(IN.worldNormal, H)), 8.0f);
    float diffuse = saturate(dot(IN.worldNormal, lightDirection));

    float3 lighting = color * (diffuse + specular);

    return float4(lighting, 1);
}