#ifndef __IMPORTANCE_SAMPLING_HLSL__
#define __IMPORTANCE_SAMPLING_HLSL__

// Dammertz 2012 - Hammersley Points on the Hemisphere
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Reverse bits in a 32 bit integer.
float RadicalInverse_VdC(uint i)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley2d(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 Hammersley3d(uint i, uint N)
{
    const float phi = 2 * PI * RadicalInverse_VdC(i);
    return float3(float(i) / float(N), cos(phi), sin(phi));
}

// Note: Sampled in tangent space, with N = (0, 0, 1)
float3 HemisphereImportanceSample_GGX(float2 Xi, float a)
{
    const float phi = 2 * PI * Xi.y;
    // Optimized for better fp accuracy: (a * a - 1) == (a + 1) * (a - 1)
    float cosTheta2 = (1 - Xi.x) / (1 + (a + 1) * ((a - 1) * Xi.x));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1 - cosTheta2);
    
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 HemisphereImportanceSample_GGX(float3 Xi, float a)
{
    // Optimized for better fp accuracy: (a * a - 1) == (a + 1) * (a - 1)
    float cosTheta2 = (1 - Xi.x) / (1 + (a + 1) * ((a - 1) * Xi.x));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1 - cosTheta2);
    
    return float3(sinTheta * Xi.y, sinTheta * Xi.z, cosTheta);
}

float2 DiskSampleUniform(float2 Xi)
{
    const float theta = 2 * PI * Xi.y;
    float radius = sqrt(Xi.x);
    return radius * float2(cos(theta), sin(theta));
}

float3 HemisphereImportanceSample_VisibleGGX(float2 Xi, float2 a, float3 V)
{
// Section 3.2: transforming the view direction to the hemisphere configuration
    float3 Vh = normalize(float3(a * V.xy, V.z));
// Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) * rsqrt(lensq) : float3(1, 0, 0);
    float3 T2 = cross(Vh, T1);
// Section 4.2: parameterization of the projected area
    float2 p = DiskSampleUniform(Xi);
    float s = Vh.z * 0.5 + 0.5;
    p.y = lerp(sqrt(1.0 - p.x * p.x), p.y, s);
// Section 4.3: reprojection onto hemisphere
    float3 Nh = p.x * T1 + p.y * T2 + sqrt(saturate(1 - dot(p, p))) * Vh;
// Section 3.4: transforming the normal back to the ellipsoid configuration
    float3 Ne = normalize(float3(a * Nh.xy, max(Nh.z, 0)));
    return Ne;
}

float3 HemisphereImportanceSample_VisibleGGX_SphericalCaps(float2 Xi, float2 a, float3 V)
{
// Section 3.2: transforming the view direction to the hemisphere configuration
    float3 Vh = normalize(float3(a * V.xy, V.z));

    float phi = 2.0f * PI * Xi.x;
    //float z = fma((1.0f - Xi.y), (1.0f + Vh.z), -Vh.z);
    float z = 1 - Xi.y - Xi.y * Vh.z;
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float3 c = float3(x, y, z);
	// compute halfway direction;
    float3 Nh = c + Vh;
    
    float3 Ne = normalize(float3(a * Nh.xy, max(Nh.z, 0)));
    return Ne;
}

float SphereSampleUniformPDF()
{
    return 1.0 / (4.0 * PI);
}

float3 SphereSampleUniform(float2 Xi)
{
    const float phi = 2 * PI * Xi.y;
    float cosTheta = Xi.x * 2.0 - 1.0;
    float sinTheta = sqrt(1 - cosTheta * cosTheta); // max(0)?
    
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float HemisphereSampleUniformPDF()
{
    return 1.0 / (2.0 * PI);
}

float3 HemisphereSampleUniform(float2 Xi)
{
    const float phi = 2 * PI * Xi.y;
    float cosTheta = Xi.x;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float HemisphereSampleCosinePDF(float cosTheta)
{
    return cosTheta * (1.0 / PI);
}

float3 HemisphereSampleCosine(float2 Xi)
{
    const float phi = 2 * PI * Xi.y;
    float cosTheta = sqrt(Xi.x);
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
        
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

#endif // __IMPORTANCE_SAMPLING_HLSL__
