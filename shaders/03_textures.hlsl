#include "Common.hlsli"
#include "Color.hlsli"

struct FrameCB
{
    float4x4 viewProjMatrix;
    float3 cameraPos;
    uint skyboxTexIdx;
};
ConstantBuffer<FrameCB> FrameConstantBuffer : register(b0);

struct TexturedMeshCB
{
    float4x4 modelMatrix;
	uint vtxBufIdx;
	uint colorTexIdx;
	uint colorSamplerIdx;
};
ConstantBuffer<TexturedMeshCB> ObjectConstantBuffer : register(b1);

struct Vtx3fPos3fNormal2fUv
{
    float3 pos;
    float3 normal;
    float2 uv;
};

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

float3 F_Schlick(float3 f0, float cosTheta)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0f);
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colorTexIdx];
    SamplerState colorSampler = SamplerDescriptorHeap[ObjectConstantBuffer.colorSamplerIdx];
    TextureCube<float4> skyboxTex = ResourceDescriptorHeap[FrameConstantBuffer.skyboxTexIdx];
    SamplerState skyboxSampler = SamplerDescriptorHeap[LinearClampSampler];

    float3 V = normalize(FrameConstantBuffer.cameraPos - IN.worldPos);
    float3 R = reflect(-V, normalize(IN.worldNormal));

    float3 baseColor = sRGBtoLinear(colorTex.Sample(colorSampler, IN.uv).rgb);
    float3 reflectionColor = sRGBtoLinear(skyboxTex.Sample(skyboxSampler, R).rgb);

    float3 F = F_Schlick(0.08f, saturate(dot(normalize(IN.worldNormal), V)));
    return float4(baseColor * (1.0f - F) + reflectionColor * F, 1.0f);
}