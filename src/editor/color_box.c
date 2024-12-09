#include "../system/microui.h"
#include "color_box.h"

int color_property_grid(struct mu_Context* gui_context, struct color_box* context)
{
    int res = 0;

    context->hsv = color4f_to_hsv(*context->rgba_output);
    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context, "color");
    mu_Rect r = mu_layout_next(gui_context);

    mu_header_ex(gui_context, "rgb", 0);
    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_draw_rect(gui_context, r, mu_color((int)(context->rgba_output->red*255.f), (int)(context->rgba_output->green*255.f), (int)(context->rgba_output->blue*255.f), 255));
    mu_label(gui_context, "red");
    res |= mu_slider_ex(gui_context, &context->rgba_output->red, 0.f, 1.f, 0.001f, "%1.2f", 0);
    mu_label(gui_context, "green");
    res |= mu_slider_ex(gui_context, &context->rgba_output->green, 0.f, 1.f, 0.001f, "%1.2f", 0);
    mu_label(gui_context, "blue");
    res |= mu_slider_ex(gui_context, &context->rgba_output->blue, 0.f, 1.f, 0.001f, "%1.2f", 0);

    mu_header(gui_context, "hsv");
    mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
    mu_label(gui_context, "");
    r = mu_layout_next(gui_context);

    float hue_step = 360.f / (float)r.w;
    hsv4f hsv = {.hue = 0.f, .saturation = 1.f, .value = 1.f};
    for(int x=0; x<r.w; ++x)
    {
        packed_color color_uint = hsv_to_packed_color(hsv);
        mu_draw_rect(gui_context, mu_rect(r.x + x, r.y, 1, r.h), 
                     mu_color(packed_color_get_red(color_uint), packed_color_get_green(color_uint), packed_color_get_blue(color_uint), 255));
        hsv.hue += hue_step;
    }

    int res_hsv = 0;
    mu_label(gui_context, "hue");
    res |= mu_slider_ex(gui_context, &context->hsv.hue, 0.f, 360.f, 1.f, "%1.0f", 0);


    return res;
}