
struct CubeCB
{
    float4x4 viewProjMatrix;
    float4x4 modelMatrix;
    uint vtxBufIdx;
    uint colTexIdx;
};

ConstantBuffer<CubeCB> ObjectConstantBuffer : register(b0);

struct CubeVertexInput
{
    float3 pos;
    float2 uv;
};

struct CubeVertexOutput
{
    float4 pos	: SV_POSITION;
    float2 uv	: TEXCOORD0;
};

CubeVertexOutput VS_Cube(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
	CubeVertexInput vtx = vtxBuf.Load<CubeVertexInput>(vtxId * sizeof(CubeVertexInput));

	CubeVertexOutput OUT;
    float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.pos, 1)).xyz;
	OUT.pos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
	OUT.uv = vtx.uv;

    return OUT;
}

float4 PS_Cube(CubeVertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colTexIdx];
	SamplerState colorSampler = SamplerDescriptorHeap[PointClampSampler];
    
    return float4(colorTex.Sample(colorSampler, IN.uv).rgb, 1); 
}
