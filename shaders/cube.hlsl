
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
    float3 normal;
    float2 uv;
};

struct CubeVertexOutput
{
    float4 pos          : SV_POSITION;
    float3 worldPos     : WORLD_POSITION;
    float3 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD0;
};

CubeVertexOutput VS_Cube(uint vtxId : SV_VertexID)
{
    ByteAddressBuffer vtxBuf = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
	CubeVertexInput vtx = vtxBuf.Load<CubeVertexInput>(vtxId * sizeof(CubeVertexInput));

	float3 worldPos = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.pos, 1)).xyz;
    float3 worldNormal = mul(ObjectConstantBuffer.modelMatrix, float4(vtx.normal, 0)).xyz;

	CubeVertexOutput OUT;
	OUT.pos = mul(ObjectConstantBuffer.viewProjMatrix, float4(worldPos, 1));
    OUT.worldPos = worldPos;
    OUT.worldNormal = worldNormal;
	OUT.uv = vtx.uv;

    return OUT;
}

float4 PS_Cube(CubeVertexOutput IN) : SV_TARGET
{
    Texture2D<float4> colorTex = ResourceDescriptorHeap[ObjectConstantBuffer.colTexIdx];
	SamplerState colorSampler = SamplerDescriptorHeap[PointClampSampler];

    return float4(colorTex.Sample(colorSampler, IN.uv).rgb, 1);
}

//

cbuffer PushConstants : register(PushConstantRegister)
{
	uint RenderTargetIdx;
};

struct FullscreenVertexOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
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

FullscreenVertexOutput VS_Fullscreen(uint vtxId : SV_VertexID)
{
	float4 vtx = MakeFullscreenTriangle(vtxId);

	FullscreenVertexOutput OUT;
	OUT.pos = float4(vtx.xy, 0, 1);
	OUT.uv = vtx.zw;

	return OUT;
}

float3 sRGBtoLinear(float3 col)
{
	return pow(col, 1.0 / 2.2);
}

float4 PS_Fullscreen(FullscreenVertexOutput IN) : SV_TARGET
{
	Texture2D<float4> colorTex = ResourceDescriptorHeap[RenderTargetIdx];
	SamplerState colorTexSampler = SamplerDescriptorHeap[PointClampSampler];

	float3 color = colorTex.Sample(colorTexSampler, IN.uv).rgb;

	return float4(sRGBtoLinear(color), 1);
}