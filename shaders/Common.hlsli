#ifndef __COMMON_HLSL__
#define __COMMON_HLSL__

float3 sRGBtoLinear(float3 col)
{
    return pow(col, 2.2f);
}

float3 ApplyGammaCorrection(float3 col)
{
    return pow(col, 1.0f / 2.2f);
}

void ComputeOrthonormalBasis(float3 N, out float3 T, out float3 B)
{
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// Pixar - Building an Orthonormal Basis, Revisited
// https://jcgt.org/published/0006/01/01/
void ComputeOrthonormalBasis_Pixar(float3 N, out float3 T, out float3 B)
{
    float sz = (N.z >= 0 ? 1 : -1);

    float a = 1.0 / (sz + N.z);
    float ya = N.y * a;
    float b = N.x * ya;
    float c = N.x * sz;

    T = float3(c * N.x * a - 1.0, sz * b, c);
    B = float3(b, N.y * ya - sz, N.y);
}

float3x3 GetTangentBasis(float3 N)
{
    float3 T, B;
    ComputeOrthonormalBasis(N, T, B);
    return float3x3(T, B, N);
}

float3 TangentToWorld(float3 vec, float3x3 TBN)
{
    return mul(vec, TBN);
}

float3 TangentToWorld(float3 vec, float3 N)
{
    return mul(vec, GetTangentBasis(N));
}

//

struct CubemapParams
{
    SamplerState cubeSampler;
    TextureCube<float4> cubeTex;
    bool bConvertToLinear;
};

float3 SampleCubemap(CubemapParams p, float3 uv, uint mip = 0)
{
    float3 color = p.cubeTex.SampleLevel(p.cubeSampler, uv, mip).rgb;
    if (p.bConvertToLinear)
    {
        color = sRGBtoLinear(color);
    }
    return color;
}

#endif // __COMMON_HLSL__