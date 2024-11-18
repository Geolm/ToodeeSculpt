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
    float saturation;
    float saturation_shift;
    float value;
    float value_shift;
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
    case 0 : return float4_to_rgba(value, t, p, alpha);
    case 1 : return float4_to_rgba(q, value, p, alpha);
    case 2 : return float4_to_rgba(p, value, t, alpha);
    case 3 : return float4_to_rgba(p, q, value, alpha);
    case 4 : return float4_to_rgba(t, p, value, alpha);
    default: return float4_to_rgba(value, p, q, alpha);
    }
}

// ----------------------------------------------------------------------------
static inline uint32_t hueshift_ramp(const hueshift_ramp_desc* desc, float t, float alpha)
{
    assert(desc->hue >= 0.f && desc->hue <= 360.f);
    assert(t >= 0.f && t <= 1.f);

    float value = desc->value + t * desc->value_shift;
    float saturation = desc->saturation + t * desc->saturation_shift;
    float hue = desc->hue + t * desc->hue_shift;

    if (hue>360.f)
        hue -= 360.f;
    else if (hue<0.f)
        hue += 360.f;

    saturation = fminf(fmaxf(saturation, 0.f), 1.f);
    value = fminf(fmaxf(value, 0.f), 1.f);

    return hsv_to_rgba(hue, saturation, value, alpha);
}

// ----------------------------------------------------------------------------
static inline uint32_t palette_ramp(const uint32_t* palette, uint32_t num_palette_entries, float t, uint8_t alpha)
{
    // clamp t
    t = fmaxf(fminf(t, 1.f), 0.f);
    uint32_t index = ((float)num_palette_entries) * t;
    return (palette[index] & 0x00ffffff) | (alpha<<24);
}

#endif
