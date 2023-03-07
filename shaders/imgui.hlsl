#include "shaders_shared.h"

cbuffer ConstantBuffer : register(b0, PerObjectSpace)
{
	float4x4 ProjectionMatrix;
};

SamplerState BilinearSampler : register(s0);
Texture2D FontTexture : register(t0);

struct VS_INPUT
{
	float2 pos : POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD0;
	float4 col : COLOR0;
};

PS_INPUT VS_Main(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));
	output.uv = input.uv;
	output.col = input.col;
	return output;
}

float4 PS_Main(PS_INPUT input) : SV_Target
{
	float4 out_col = input.col * FontTexture.Sample(BilinearSampler, input.uv);
	return out_col;
	//return float4(pow(out_col.rgb, 1.0/ 2.2), out_col.a);
}
