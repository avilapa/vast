#define EPS		        0.0001f
#define PI              3.14159265358979323846264338327950288
#define INV_PI          (1.0 / PI)
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
