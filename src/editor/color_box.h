#ifndef __COLOR_BOX__H__
#define __COLOR_BOX__H__

#include "../system/color.h"

struct mu_Context;


struct color_box
{
    color4f* rgba_output;
    hsv4f hsv;
    packed_color* palette_entries;
    uint32_t num_palette_entries;
};

#ifdef __cplusplus
extern "C" {
#endif

// returns MU_RES_SUBMIT if the color has changed
int color_property_grid(struct mu_Context* gui_context, struct color_box* color);

#ifdef __cplusplus
}
#endif



#endif