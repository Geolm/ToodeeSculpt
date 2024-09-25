// based on https://iquilezles.org/articles/smin/


// ---------------------------------------------------------------------------------------------------------------------------
// quadratic polynomial
// returns
//  .x = smallest distance
//  .y = blend factor between [0; 1]
float2 smooth_minimum(float a, float b, float k)
{
    if (k>0.f)
    {
        float h = 1.0f - min( abs(a-b)/(4.0f*k), 1.0f);
        float w = h*h;
        float m = w*0.5f;
        float s = w*k;
        return (a<b) ? float2(a-s,m) : float2(b-s,1.0f-m);
    }
    else
    {
        // "hard" min
        return float2(min(a, b),  (a<b) ? 0.f : 1.f);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
float smooth_substraction(float d1, float d2, float k)
{
    if (k>0.f)
    {
        float h = saturate(0.5f - 0.5f*(d2+d1)/k);
        return mix(d2, -d1, h) + k*h*(1.0f-h);
    }
    else
    {
        return max(-d1, d2);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
float smooth_intersection(float d1, float d2, float k )
{
    if (k>0.f)
    {
        float h = saturate(0.5f - 0.5f*(d2-d1)/k);
        return mix(d2, d1, h) + k*h*(1.0-h);
    }
    else
    {
        return max(d1, d2);
    }
}