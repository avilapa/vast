
cbuffer PushConstants : register(PushConstantRegister)
{
    uint TriangleVtxBufIdx;
};

struct VertexInput
{
	float2 pos;
	float3 col;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

VertexOutput VS_Main(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[TriangleVtxBufIdx];
    VertexInput vtx = vtxBuf.Load<VertexInput>(vtxId * sizeof(VertexInput));
    
    VertexOutput OUT;
    OUT.pos = float4(vtx.pos, 0, 1);
    OUT.col = vtx.col;
    
    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{
    return float4(IN.col, 1);
}