
struct InstanceData
{
    float4x4 modelMatrix;
    float3 color;
    float __pad0;
};
StructuredBuffer<InstanceData> InstanceBuffer : register(t0);

struct CubeCB
{
    float4x4 viewProjMatrix;
    uint vtxBufIdx;
};
ConstantBuffer<CubeCB> ObjectConstantBuffer : register(b0);

struct VertexOutput
{
    float4 pos  : SV_Position;
    float3 col  : Color0;
};

VertexOutput VS_Main(uint vtxId : SV_VertexID, uint instId : SV_InstanceID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
    float3 vtx = vtxBuf.Load<float3>(vtxId * sizeof(float3));
    
    VertexOutput OUT;
    float3 worldPos = mul(InstanceBuffer[instId].modelMatrix, float4(vtx, 1)).xyz;
    OUT.pos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
    OUT.col = InstanceBuffer[instId].color;
    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_Target
{
    return float4(IN.col, 1);
}
