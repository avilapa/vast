#define EPS		        0.0001f
#define PI              3.14159265358979323846264338327950288
#define INV_PI          (1.0 / PI)
#define TWO_PI          (2.0 * PI)
#define FOUR_PI         (4.0 * PI)
#define HALF_PI         (0.5 * PI)
#define MIN_ROUGHNESS   0.0002

#define BSDF_USE_GGX_MULTISCATTER 1

float Pow5(float v)
{
    float v2 = v * v;
    return v2 * v2 * v;
}

float LinearizeRoughness(float roughness)
{
    return roughness * roughness;
}

// GGX was originally introduced in Walter 2007, in a similar form to the one in Karis 2013, shown
// below:
//
//	    float a2 = roughness * roughness;
//	    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
//	    return INV_PI * a2 / (t * t);
//
// This form is equivalent to the one in D_GGX (https://www.desmos.com/calculator/fz0sgowq46),
// taken from Guy & Agopian 2018.
//
// Walter 2007 - Microfacet Models for Refraction through Rough Surfaces
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
// Karis 2013 - Specular BRDF Reference
// https://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// Guy & Agopian 2018 - Physically Based Rendering in Filament
// https://google.github.io/filament/Filament.html
//
float D_GGX(float NdotH, float roughness)
{
    float a = NdotH * roughness;
    float k = roughness / (1.0 - NdotH * NdotH + a * a);
    return k * k * INV_PI;
}

float V_SmithGGXHeightCorrelated(float NdotV, float NdotL, float roughness)
{
    float a2 = roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float3 F_Schlick(float3 f0, float cosTheta)
{
    return f0 + (1.0 - f0) * Pow5(1.0 - cosTheta);
}

float3 GetEnergyCompensationGGX(float3 f0, float2 dfg)
{
#if BSDF_USE_GGX_MULTISCATTER
	return 1.0.xxx + f0 * (1.0 / dfg.y - 1.0);
#else
    return 1.0.xxx;
#endif
}
