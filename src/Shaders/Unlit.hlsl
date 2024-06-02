#include "shaders_shared.h"
#include "VertexInputs_shared.h"
#include "Common.hlsli"
#include "Color.hlsli"

ConstantBuffer<UnlitCB> ObjectConstantBuffer : register(b1, PerObjectSpace);

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

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    return UnpackColorFromUint(ObjectConstantBuffer.color);
}