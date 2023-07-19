
#if !defined(CUBE_VERTEX_NO_UV)
#define CUBE_HAS_UV 1
#else
#define CUBE_HAS_UV 0
#endif

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
#if CUBE_HAS_UV
    float2 uv;
#endif
};

struct CubeVertexOutput
{
    float4 pos	: SV_POSITION;
#if CUBE_HAS_UV
    float2 uv	: TEXCOORD0;
#endif
};

CubeVertexOutput VS_Cube(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
	CubeVertexInput vtx = vtxBuf.Load<CubeVertexInput>(vtxId * sizeof(CubeVertexInput));

	CubeVertexOutput OUT;
    float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.pos, 1)).xyz;
	OUT.pos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
#if CUBE_HAS_UV
	OUT.uv = vtx.uv;
#endif
    return OUT;
}

float4 PS_Cube(CubeVertexOutput IN) : SV_TARGET
{
#if CUBE_HAS_UV
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colTexIdx];
	SamplerState colorSampler = SamplerDescriptorHeap[PointClampSampler];
    
    return float4(colorTex.Sample(colorSampler, IN.uv).rgb, 1); 
#else
    return float4(IN.pos.rgb, 1);
#endif
}
