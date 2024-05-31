
cbuffer ImguiCB : register(b0)
{
	float4x4 ProjectionMatrix;
};

cbuffer TextureSRV : register(PushConstantRegister)
{
    uint TextureIdx;
};

struct VertexInputImgui
{
	float2 pos : POSITION;
	float2 uv  : TEXCOORD0;
	uint   col : COLOR0;
};

struct VertexOutputImgui
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;
};

float4 UnpackColorFromUint(uint v) 
{
	return uint4(v & 255, (v >> 8) & 255, (v >> 16) & 255, (v >> 24) & 255) * (1.0f / 255.0f);
}

VertexOutputImgui VS_Imgui(VertexInputImgui IN)
{
    VertexOutputImgui OUT;
    OUT.pos = mul(ProjectionMatrix, float4(IN.pos.xy, 0.0f, 1.0f));
    OUT.uv = IN.uv;
	// Note: ImGui stores color as uint and sets the InputLayout format to DXGI_FORMAT_R8G8B8A8_UNORM,
	// but then declares the color in the vertex input be viewed as a float4. Since we get our Input
	// Layouts from shader reflection we cannot make this distinction, so we do the conversion manually.
    OUT.col = UnpackColorFromUint(IN.col);
    return OUT;
}

float4 PS_Imgui(VertexOutputImgui IN) : SV_TARGET
{
    Texture2D<float4> tex = ResourceDescriptorHeap[TextureIdx];
	SamplerState sam = SamplerDescriptorHeap[LinearClampSampler];
    return IN.col * tex.Sample(sam, IN.uv);
}
