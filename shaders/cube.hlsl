
struct CubeCB
{
    float4x4 viewProjMatrix;
    float4x4 modelMatrix;
    uint vtxBufIdx;
};
ConstantBuffer<CubeCB> ObjectConstantBuffer : register(b0);

float4 UnpackColorFromUint(uint p)
{
    return float4((p & 0xff) / 255.0f, ((p >> 8) & 0xff) / 255.0f, ((p >> 16) & 0xff) / 255.0f, ((p >> 24) & 0xff) / 255.0f);
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

VertexOutput VS_Cube(uint vtxId : SV_VERTEXID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
	float3 pos = vtxBuf.Load<float3>(vtxId * sizeof(float3));
    float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(pos, 1)).xyz;

    VertexOutput OUT;
    OUT.worldPos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
    OUT.color = UnpackColorFromUint(WangHash(vtxId));
    
    return OUT;
}

float4 PS_Cube(VertexOutput IN) : SV_TARGET
{   
    return IN.color;
}
