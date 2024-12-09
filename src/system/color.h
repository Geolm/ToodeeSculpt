#ifndef __COLOR_RAMP__
#define __COLOR_RAMP__

#include <stdint.h>

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

typedef struct
{
    float red, green, blue, alpha;
} color4f;

typedef struct
{
    float hue, saturation, value, alpha;
} hsv4f;

typedef uint32_t packed_color;


#ifdef __cplusplus
extern "C" {
#endif


packed_color color4f_to_packed_color(color4f color);
color4f hsv_to_color4f(hsv4f color);
packed_color hsv_to_packed_color(hsv4f color);
packed_color hueshift_ramp(const hueshift_ramp_desc* desc, float t, float alpha);
packed_color palette_ramp(const packed_color* palette, uint32_t num_palette_entries, float t, uint8_t alpha);

#ifdef __cplusplus
}
#endif

#endif
