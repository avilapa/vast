cbuffer PushConstants : register(PushConstantRegister)
{
	uint RenderTargetIdx;
};

struct VertexOutputFS
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
};

float4 MakeFullscreenTriangle(int vtxId)
{
    // 0 -> pos = [-1, 1], uv = [0,0]
    // 1 -> pos = [-1,-3], uv = [0,2] 
    // 2 -> pos = [ 3, 1], uv = [2,0]
    float4 vtx;
    vtx.zw = float2(vtxId & 2, (vtxId << 1) & 2);
    vtx.xy = vtx.zw * float2(2, -2) + float2(-1, 1);
    return vtx;
}

VertexOutputFS VS_Main(uint vtxId : SV_VertexID)
{
    float4 vtx = MakeFullscreenTriangle(vtxId);

    VertexOutputFS OUT;
    OUT.pos = float4(vtx.xy, 0, 1);
    OUT.uv = vtx.zw;

    return OUT;
}

float3 sRGBtoLinear(float3 col)
{
    return pow(col, 1.0 / 2.2);
}

float4 PS_Main(VertexOutputFS IN) : SV_TARGET
{
    Texture2D<float4> rt = ResourceDescriptorHeap[RenderTargetIdx];
    SamplerState rtSampler = SamplerDescriptorHeap[PointClampSampler]; 

    float3 color = rt.Sample(rtSampler, IN.uv).rgb;

    return float4(sRGBtoLinear(color), 1);
}