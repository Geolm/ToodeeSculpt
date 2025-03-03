// based on https://iquilezles.org/articles/smin/


// ---------------------------------------------------------------------------------------------------------------------------
// smooth minimum : quadratic polynomial
//
// note : a primitive is on top of b primitive. Also it handle a anti-aliased primitive going over another primitive and keeping 
//        the right width of anti-aliasing 
//
// returns
//  .x = smallest distance
//  .y = blend factor between [0; 1]
float2 smooth_minimum(float a, float b, float k)
{
    b = max(b, 0.f);    // a is always on top
    if (k>0.f)
    {
        float h = max( k-abs(a-b), 0.0f )/k;
        float m = h*h*h*0.5;
        float s = m*k*(1.0f/3.0f); 
        return (a<b) ? float2(a-s, 0.f) : float2(b-s, 1.f - smoothstep(-k, 0.f, b-a));
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
        return mix(d1, -d2, h) + k*h*(1.0f-h);
    }
    else
    {
        return max(-d2, d1);
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