#include "shaders_shared.h"

cbuffer PushConstants : register(PushConstantRegister, PerObjectSpace)
{
	float4x4 ProjectionMatrix;
};

SamplerState BilinearSampler : register(s0);
Texture2D FontTexture : register(t0);

struct VertexInput
{
	float2 pos : POSITION;
	float2 uv  : TEXCOORD0;
	uint   col : COLOR0;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;
};

float4 UnpackColorFromUint(uint v) 
{
	return uint4(v & 255, (v >> 8) & 255, (v >> 16) & 255, (v >> 24) & 255) * (1.0f / 255.0f);
}

VertexOutput VS_Main(VertexInput input)
{
	VertexOutput output;
	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));
	output.uv = input.uv;
	// TODO: We don't currently support custom InputLayouts. ImGui uses a float4 for vertex input 
	// color, and sets DXGI_FORMAT_R8G8B8A8_UNORM in the input layout. Therefore the conversion 
	// happens implicitly.
	output.col = UnpackColorFromUint(input.col);
	return output;
}

float4 PS_Main(VertexOutput input) : SV_Target
{
	float4 out_col = input.col * FontTexture.Sample(BilinearSampler, input.uv);
	return out_col;
}
