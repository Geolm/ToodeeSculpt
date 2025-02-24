#include "../system/microui.h"
#include "../system/palettes.h"
#include "../system/nfd.h"
#include "color_box.h"
#include <assert.h>
#include <stdlib.h>

#define UNUSED_VARIABLE(a) (void)(a)

//----------------------------------------------------------------------------------------------------------------------------
mu_Color hue_gradient(mu_Real value, void* user_data)
{
    UNUSED_VARIABLE(user_data);
    hsv4f hsv = {.hue = value * 360.f, .saturation = 1.f, .value = 1.f};
    packed_color color_uint = hsv_to_packed_color(hsv);
    return mu_color(packed_color_get_red(color_uint), packed_color_get_green(color_uint), packed_color_get_blue(color_uint), 255);
}

//----------------------------------------------------------------------------------------------------------------------------
mu_Color saturation_gradient(mu_Real value, void* user_data)
{
    hsv4f hsv = *((hsv4f*) user_data);
    hsv.saturation = value;
    packed_color color_uint = hsv_to_packed_color(hsv);
    return mu_color(packed_color_get_red(color_uint), packed_color_get_green(color_uint), packed_color_get_blue(color_uint), 255);
}

//----------------------------------------------------------------------------------------------------------------------------
mu_Color value_gradient(mu_Real value, void* user_data)
{
    hsv4f hsv = *((hsv4f*) user_data);
    hsv.value = value;
    packed_color color_uint = hsv_to_packed_color(hsv);
    return mu_color(packed_color_get_red(color_uint), packed_color_get_green(color_uint), packed_color_get_blue(color_uint), 255);
}

//----------------------------------------------------------------------------------------------------------------------------
int color_property_grid(struct mu_Context* gui_context, struct color_box* context)
{
    int res = 0;

    context->hsv = color4f_to_hsv(*context->rgba_output);
    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context, "color");
    mu_layout_height(gui_context, 50);
    mu_Rect r = mu_layout_next(gui_context);
    mu_draw_rect(gui_context, r, mu_color((int)(context->rgba_output->red*255.f), (int)(context->rgba_output->green*255.f), (int)(context->rgba_output->blue*255.f), 255));

    if (mu_header_ex(gui_context, "rgb", MU_OPT_EXPANDED))
    {
        mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
        mu_label(gui_context, "red");
        res |= mu_slider_ex(gui_context, &context->rgba_output->red, 0.f, 1.f, 0.001f, "%1.2f", 0);
        mu_label(gui_context, "green");
        res |= mu_slider_ex(gui_context, &context->rgba_output->green, 0.f, 1.f, 0.001f, "%1.2f", 0);
        mu_label(gui_context, "blue");
        res |= mu_slider_ex(gui_context, &context->rgba_output->blue, 0.f, 1.f, 0.001f, "%1.2f", 0);
    }

    if (mu_header_ex(gui_context, "hsv", MU_OPT_EXPANDED))
    {
        mu_layout_row(gui_context, 2, (int[]) { 120, -1 }, 0);

        mu_label(gui_context, "hue");
        int res_hsv = mu_slider_gradient(gui_context, &context->hsv.hue, 0.f, 360.f, 1.f, "%1.0f", 0, hue_gradient, NULL);

        mu_label(gui_context, "saturation");
        res_hsv |= mu_slider_gradient(gui_context, &context->hsv.saturation, 0.f, 1.f, 0.01f, "%1.2f", 0, saturation_gradient, &context->hsv);

        mu_label(gui_context, "value");
        res_hsv |= mu_slider_gradient(gui_context, &context->hsv.value, 0.f, 1.f, 0.01f, "%1.2f", 0, value_gradient, &context->hsv);

        if (res_hsv&MU_RES_CHANGE)
            *context->rgba_output = hsv_to_color4f(context->hsv);
        
        res |= res_hsv;
    }

    if (mu_header_ex(gui_context, "palette", MU_OPT_EXPANDED))
    {
        assert(context->palette->num_entries <= 32);
        r = mu_layout_next(gui_context);

        int width = r.w / context->palette_entries_per_row;

        for(uint32_t i=0; i<context->palette->num_entries; ++i)
        {
            uint32_t x = i%context->palette_entries_per_row;
            if (x==0 && i > 0)
                r = mu_layout_next(gui_context);

            packed_color entry = context->palette->entries[i];
            mu_Rect entry_rect = mu_rect(r.x + x*width, r.y, width, r.h);

            if (mu_mouse_over(gui_context, entry_rect))
            {
                if (gui_context->mouse_pressed&MU_MOUSE_LEFT)
                {
                    // select palette entry
                    *context->rgba_output = unpacked_color(entry);
                    res |= MU_RES_SUBMIT;

                    entry_rect.w += 4; entry_rect.h += 4;
                    entry_rect.x -= 2; entry_rect.y -= 2;
                }
                else if ((gui_context->mouse_pressed&MU_MOUSE_RIGHT))
                {
                    // overwrite palette entry
                    context->palette->entries[i] = color4f_to_packed_color(*context->rgba_output);
                    res |= MU_RES_SUBMIT;
                }
                else
                {
                    mu_draw_rect(gui_context, entry_rect, gui_context->style->colors[MU_COLOR_BASE]);
                    entry_rect.w -= 4; entry_rect.h -= 4;
                    entry_rect.x += 2; entry_rect.y += 2;
                }
            }

            mu_draw_rect(gui_context, entry_rect, mu_color(packed_color_get_red(entry), packed_color_get_green(entry), packed_color_get_blue(entry), 255));
        }

        if (mu_button(gui_context, "load"))
        {
            nfdchar_t *load_path = NULL;
            nfdresult_t result = NFD_OpenDialog( "hex", NULL, &load_path);

            if (result == NFD_OKAY)
                palette_load_from_hex(load_path, context->palette);
                
            free(load_path);
        }

    }

    return res;
}