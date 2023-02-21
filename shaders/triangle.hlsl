#include "shaders_shared.h"

ConstantBuffer<TriangleCBV> ObjectConstantBuffer : register(b0, space0);

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

VertexOutput VS_Main(uint vertexId : SV_VertexID)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[ObjectConstantBuffer.vtxBufIdx];
    TriangleVtx vertex = vertexBuffer.Load<TriangleVtx>(vertexId * sizeof(TriangleVtx));
    
    VertexOutput output;
    output.pos = float4(vertex.pos, 0, 1);
    output.col = vertex.col;
    
    return output;
}

float4 PS_Main(VertexOutput input) : SV_TARGET
{
    return float4(input.col, 1);
}