#ifndef __COLOR_BOX__H__
#define __COLOR_BOX__H__

#include "../system/color.h"
#include "../system/palettes.h"

struct mu_Context;


struct color_box
{
    color4f* rgba_output;
    hsv4f hsv;
    struct palette *palette;
    uint32_t palette_entries_per_row;
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