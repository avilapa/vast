#include "shaders_shared.h"

cbuffer vertexBuffer : register(b0, PerObjectSpace)
{
	float4x4 ProjectionMatrix;
};

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
	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
	output.uv = input.uv;
	output.col = input.col;
	return output;
}

SamplerState sampler0 : register(s0);
Texture2D texture0 : register(t0);

float4 PS_Main(PS_INPUT input) : SV_Target
{
	float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
	return out_col;
	//return float4(pow(out_col.rgb, 1.0/ 2.2), out_col.a);
}
