#include <assert.h>
#include <math.h>
#include "color.h"

// ----------------------------------------------------------------------------
packed_color color4f_to_packed_color(color4f color)
{
    uint32_t result = ((uint32_t)(color.alpha * 255.f)) << 24;
    result |= ((uint32_t)(color.blue * 255.f)) << 16;
    result |= ((uint32_t)(color.green * 255.f)) << 8;
    result |= ((uint32_t)(color.red * 255.f));
    return result;
}

// ----------------------------------------------------------------------------
color4f hsv_to_color4f(hsv4f color)
{
    assert(color.hue >= 0.f && color.hue <= 360.f);
    assert(color.saturation >= 0.f && color.saturation <= 1.f);
    assert(color.value >= 0.f && color.value <= 1.f);
    
    float hue_over_60 = color.hue / 60.f;
    int index = (int)hue_over_60;

    float frac_hue = hue_over_60 - index;
    float p = color.value * (1.f - color.saturation);
    float q = color.value * (1.f - (color.saturation * frac_hue));
    float t = color.value * (1.f - (color.saturation * (1.f - frac_hue)));

    switch(index)
    {
    case 0 : return (color4f){color.value, t, p, color.alpha};
    case 1 : return (color4f){q, color.value, p, color.alpha};
    case 2 : return (color4f){p, color.value, t, color.alpha};
    case 3 : return (color4f){p, q, color.value, color.alpha};
    case 4 : return (color4f){t, p, color.value, color.alpha};
    default: return (color4f){color.value, p, q, color.alpha};
    }
}

// ----------------------------------------------------------------------------
packed_color hsv_to_packed_color(hsv4f color)
{
    return color4f_to_packed_color(hsv_to_color4f(color));
}

// ----------------------------------------------------------------------------
hsv4f color4f_to_hsv(color4f color)
{
    float max = (color.red > color.green) ? (color.red > color.blue ? color.red : color.blue) : (color.green > color.blue ? color.green : color.blue);
    float min = (color.red < color.green) ? (color.red < color.blue ? color.red : color.blue) : (color.green < color.blue ? color.green : color.blue);
    float delta = max - min;

    hsv4f result;
    result.alpha = color.alpha;
    result.value = max;

    if (max == 0.f) 
    {
        result.saturation = 0.f;
        result.hue = 0.f;
    }
    else
    {
        result.saturation = delta / max;

        if (delta == 0.f) 
            result.hue = 0.f;
        else if (max == color.red) 
            result.hue = (color.green - color.blue) / delta;
        else if (max == color.green) 
            result.hue = 2.0f + (color.blue - color.red) / delta;
        else 
            result.hue = 4.0f + (color.red - color.green) / delta;

        result.hue /= 6.0f;
        if (result.hue < 0.0f) 
            result.hue += 1.0f;
    }

    result.hue *= 360.f;

    return result;
}

// ----------------------------------------------------------------------------
packed_color hueshift_ramp(const hueshift_ramp_desc* desc, float t, float alpha)
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

    return hsv_to_packed_color((hsv4f){hue, saturation, value, alpha});
}

// ----------------------------------------------------------------------------
packed_color palette_ramp(const packed_color* palette, uint32_t num_palette_entries, float t, uint8_t alpha)
{
    // clamp t
    t = fmaxf(fminf(t, 1.f), 0.f);
    uint32_t index = ((float)num_palette_entries) * t;
    return (palette[index] & 0x00ffffff) | (alpha<<24);
}