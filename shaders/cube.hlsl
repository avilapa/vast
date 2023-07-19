
struct CubeCB
{
    float4x4 viewProjMatrix;
    float4x4 modelMatrix;
    uint vtxBufIdx;
    uint colTexIdx;
};

ConstantBuffer<CubeCB> ObjectConstantBuffer : register(b0);

struct Vtx3fPos2fUv
{
    float3 pos;
    float2 uv;
};

struct VertexOutput
{
    float4 pos	: SV_Position;
    float2 uv	: TexCoord0;
};

VertexOutput VS_Cube(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
	Vtx3fPos2fUv vtx = vtxBuf.Load<Vtx3fPos2fUv>(vtxId * sizeof(Vtx3fPos2fUv));

    VertexOutput OUT;
    float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.pos, 1)).xyz;
	OUT.pos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
	OUT.uv = vtx.uv;

    return OUT;
}

float4 PS_Cube(VertexOutput IN) : SV_Target
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colTexIdx];
	SamplerState colorSampler = SamplerDescriptorHeap[PointClampSampler];
    
    return float4(colorTex.Sample(colorSampler, IN.uv).rgb, 1); 
}
