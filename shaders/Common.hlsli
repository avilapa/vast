#ifndef __COMMON_HLSL__
#define __COMMON_HLSL__

#include "Color.hlsli"

// - Transformations and Matrices -------------------------------------------------------------- //

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

// - Vertex Shading ---------------------------------------------------------------------------- //

// How do we end up with back-facing normals to begin with? Interpolated/smoothed normals in the 
// mesh as an approximation of a higher polygon count mesh can cause a triangle to be visible at a
// grazing angle with a front-facing geometry normal but a back-facing vertex normal, which will be
// used for shading. Back-facing normals can lead to visual artifacts when doing lighting. A higher
// polygon count and not exporting smoothed normals prevents this issue from happening.
//
// This issue is closely related to the 'shadow-ray terminator problem'. Furtrher reading:
// Woo et al. 1996 - It's Really Not a Rendering Bug, You See...
// https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=7986ebd7b5427bbe54edb7c0c9cf674fee817933
// Schussler et al. 2017 - Microfacet-based Normal Mapping for Robust Monte Carlo Path Tracing
// https://jo.dreggn.org/home/2017_normalmap.pdf
// Chiang, Li & Burley 2019 - Taming the Shadow Terminator
// https://www.yiningkarlli.com/projects/shadowterminator.html
// Hanika 2021 - Hacking the Shadow Terminator
// https://jo.dreggn.org/home/2021_terminator.pdf

// Clamp normal directions facing away from the camera to their perpendicular direction.
float3 AdjustBackFacingVertexNormals(float3 N, float3 V)
{
    const float NdotV = dot(N, V);
    if (NdotV < 0.0f)
    {
        // Project current normal onto the plane defined by the view vector to find the closest
        // valid orientation and normalize.
        return normalize(N - V * NdotV);
    }
    return N;
}

// - Misc -------------------------------------------------------------------------------------- //

float RMSE(float forecasted, float observed)
{
    float difference = forecasted - observed;
    return sqrt(difference * difference);
}

#endif // __COMMON_HLSL__