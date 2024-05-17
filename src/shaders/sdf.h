//-----------------------------------------------------------------------------
float sd_disc(float2 position, float2 center, float radius)
{
    return length(center-position) - radius;
}

//-----------------------------------------------------------------------------
float sd_segment(float2 position, float2 a, float2 b)
{
    float2 pa = position-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.f, 1.f);
    return length(pa - ba*h);
}