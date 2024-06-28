#ifndef __COLOR_RAMP__
#define __COLOR_RAMP__

#include <stdint.h>
#include <math.h>
#include <assert.h>

// ----------------------------------------------------------------------------
typedef struct 
{
    float hue;  // [0; 360]
    float hue_shift;
    float saturation_min;
    float saturation_max;
    float value_min;
    float value_max;
} hueshift_ramp_desc;

// ----------------------------------------------------------------------------
static inline uint32_t float4_to_rgba(float r, float g, float b, float a)
{
    uint32_t result = ((uint32_t)(a * 255.f)) << 24;
    result |= ((uint32_t)(b * 255.f)) << 16;
    result |= ((uint32_t)(g * 255.f)) << 8;
    result |= ((uint32_t)(r * 255.f));
    return result;
}

// ----------------------------------------------------------------------------
static inline uint32_t hsv_to_rgba(float hue, float saturation, float value, float alpha)
{
    assert(hue >= 0.f && hue <= 360.f);
        assert(saturation >= 0.f && saturation <= 1.f);
        assert(value >= 0.f && value <= 1.f);
        
        float hue_over_60 = hue / 60.f;
        int index = (int)hue_over_60;

        float frac_hue = hue_over_60 - index;
        float p = value * (1.f - saturation);
        float q = value * (1.f - (saturation * frac_hue));
        float t = value * (1.f - (saturation * (1.f - frac_hue)));

        switch(index)
        {
        case 0: return float4_to_rgba(value, t, p, alpha);
        case 1: return float4_to_rgba(q, value, p, alpha);
        case 2: return float4_to_rgba(p, value, t, alpha);
        case 3: return float4_to_rgba(p, q, value, alpha);
        case 4: return float4_to_rgba(t, p, value, alpha);
        default: return float4_to_rgba(value, p, q, alpha);
        }
}

// ----------------------------------------------------------------------------
uint32_t hueshift_ramp(hueshift_ramp_desc* desc, float t, float alpha)
{
    assert(desc->hue >= 0.f && desc->hue <= 360.f);
    assert(desc->hue_shift >= 0.f && desc->hue_shift <= 180.f);
    assert(desc->saturation_min >= 0.f && desc->saturation_max <= 1.f);
    assert(t >= 0.f && t <= 1.f);
    assert(desc->value_min < desc->value_max);
    
    float value = fmaf(desc->value_max - desc->value_min, t, desc->value_min);
    float saturation = fmaf(desc->saturation_max - desc->saturation_min, t, desc->saturation_min);

    t = (t * 2.f) - 1.f;
    float hue = desc->hue + (t * desc->hue_shift);

    if (hue>360.f)
        hue -= 360.f;
    else if (hue<0.f)
        hue += 360.f;
        
    return hsv_to_rgba(hue, saturation, value, alpha);
}

#endif
