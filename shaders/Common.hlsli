
float3 sRGBtoLinear(float3 col)
{
    return pow(col, 2.2f);
}

float3 ApplyGammaCorrection(float3 col)
{
    return pow(col, 1.0f / 2.2f);
}
