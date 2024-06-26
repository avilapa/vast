
struct CubeCB
{
    float4x4 viewProjMatrix;
    float4x4 modelMatrix;
    uint vtxBufIdx;
};
ConstantBuffer<CubeCB> ObjectConstantBuffer : register(b0);

float4 UnpackColorFromUint(uint v)
{
    uint4 color;
    color.r = (v >> 24) & 0xFF;
    color.g = (v >> 16) & 0xFF;
    color.b = (v >> 8) & 0xFF;
    color.a = v & 0xFF;
    return color * (1.0f / 255.0f);
}

uint WangHash(uint seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15u);
    return seed;
}

struct VertexOutput
{
    float4 worldPos : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput VS_Main(uint vtxId : SV_VERTEXID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
	float3 pos = vtxBuf.Load<float3>(vtxId * sizeof(float3));
    float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(pos, 1)).xyz;

    VertexOutput OUT;
    OUT.worldPos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
    OUT.color = UnpackColorFromUint(WangHash(vtxId));
    
    return OUT;
}

float4 PS_Main(VertexOutput IN) : SV_TARGET
{   
    return IN.color;
}
