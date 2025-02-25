#include "icons.h"
#include "system/stateless.h"
#include "system/vec2.h"
#include "renderer/renderer.h"

#define UNUSED_VARIABLE(a) (void)(a)

void DrawIcon(struct renderer* gfx_context, aabb box, enum icon_type icon, draw_color primary_color, draw_color secondary_color, float time_in_second)
{
    UNUSED_VARIABLE(time_in_second);
    vec2 center = vec2_scale(vec2_add(box.min, box.max), .5f);
    vec2 extent = vec2_scale(vec2_sub(box.max, box.min), .5f);
    float max_radius = float_min(extent.x, extent.y);

    float loop_1s = sl_loop(time_in_second, 1.f);
    float loop_2s = sl_loop(time_in_second, 2.f);

    switch(icon)
    {
    case ICON_CLOSE:    // a disc with a cross
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .5f);
            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_disc(gfx_context, center, max_radius * .8f, -1.f, fill_solid, primary_color, op_union);
            renderer_draw_orientedbox(gfx_context, safe_box.min, safe_box.max, max_radius * .1f, 0.f, 0.f, fill_solid, secondary_color, op_union);
            renderer_draw_orientedbox(gfx_context, aabb_get_vertex(&safe_box, aabb_bottom_left), aabb_get_vertex(&safe_box, aabb_top_right),
                                     max_radius * .1f, 0.f, 0.f, fill_solid, secondary_color, op_union);
            renderer_end_combination(gfx_context, false);
            break;
        }
    case ICON_COLLAPSED:    // a triangle pointing to the right
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .4f);
            renderer_draw_triangle(gfx_context, aabb_get_vertex(&safe_box, aabb_bottom_left), 
                                   aabb_get_vertex(&safe_box, aabb_top_left), center, 0.f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_EXPANDED:     // a triangle pointing to the bottom
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .4f);
            renderer_draw_triangle(gfx_context, aabb_get_vertex(&safe_box, aabb_top_left),  aabb_get_vertex(&safe_box, aabb_top_right),
                                   center, 0.f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_CHECK:
        {
            vec2 a = vec2_add(center, vec2_scale((vec2){ 0.7f, -0.6f}, max_radius));
            vec2 b = vec2_add(center, vec2_scale((vec2){-0.2f,  0.5f}, max_radius));
            vec2 c = vec2_add(center, vec2_scale((vec2){-0.65f, 0.0f}, max_radius));
            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_orientedbox(gfx_context, a, b, 0.f, max_radius * .1f, 0.f, fill_solid, primary_color, op_union);
            renderer_draw_orientedbox(gfx_context, b, c, 0.f, max_radius * .1f, 0.f, fill_solid, primary_color, op_union);
            renderer_end_combination(gfx_context, false);
            break;
        }
    case ICON_DISC:
        {
            float radius = sl_wave_base(loop_2s, .4f, .3f) * max_radius;
            renderer_draw_disc(gfx_context, center, radius, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_ORIENTEDBOX:
        {
            renderer_draw_orientedbox(gfx_context, aabb_bilinear(&box, vec2_set(.8f, .2f)), aabb_bilinear(&box, vec2_set(.2f, .8f)),
                                      max_radius * 0.3f, max_radius * 0.05f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_ELLIPSE:
        {
            renderer_draw_ellipse(gfx_context, aabb_bilinear(&box, vec2_set(.3f, .2f)), aabb_bilinear(&box, vec2_set(.7f, .8f)), 
                                  max_radius * 0.5f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_PIE:
        {
            renderer_draw_pie(gfx_context, center, aabb_bilinear(&box, vec2_set(.8f, .7f)), VEC2_PI * .75f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_ARC:
        {
            renderer_draw_arc_from_circle(gfx_context, aabb_bilinear(&box, vec2_set(.7f, .7f)), aabb_bilinear(&box, vec2_set(.8f, .5f)),
                                        aabb_bilinear(&box, vec2_set(.3f, .2f)), max_radius * 0.15f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_TRIANGLE:
        {
            vec2 p0 = vec2_add(vec2_rotate(vec2_set(.3f, .3f), vec2_angle(time_in_second)), vec2_splat(.5f));
            vec2 p1 = vec2_add(vec2_rotate(vec2_set(-.1f, -.3f), vec2_angle(time_in_second * -.5f)), vec2_splat(.5f));
            vec2 p2 = vec2_add(vec2_rotate(vec2_set(-.2f, .1f), vec2_angle(time_in_second * 1.6f)), vec2_splat(.5f));
            renderer_draw_triangle(gfx_context, aabb_bilinear(&box, p0), aabb_bilinear(&box, p1), aabb_bilinear(&box, p2),
                                   max_radius * 0.05f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_CAPSULE:
        {
            renderer_draw_unevencapsule(gfx_context, aabb_bilinear(&box, vec2_set(.7f, .7f)), aabb_bilinear(&box, vec2_set(.2f, .2f)), 
                                        max_radius*0.35f, max_radius*0.15f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_SPLINE:
        {
            float thickness = 0.025374f * max_radius;
            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_arc_from_circle(gfx_context, aabb_bilinear(&box, vec2_set(.3f, .3f)), aabb_bilinear(&box, vec2_set(.5f, .4f)), 
                                        aabb_bilinear(&box, vec2_set(.5f, .5f)), thickness, fill_solid, primary_color, op_union);
            renderer_draw_arc_from_circle(gfx_context, aabb_bilinear(&box, vec2_set(.7f, .7f)), aabb_bilinear(&box, vec2_set(.5f, .6f)), 
                                        aabb_bilinear(&box, vec2_set(.5f, .5f)), thickness, fill_solid, primary_color, op_union);
            renderer_draw_arc_from_circle(gfx_context, aabb_bilinear(&box, vec2_set(.7f, .7f)), aabb_bilinear(&box, vec2_set(.9f, .8f)),
                                          aabb_bilinear(&box, vec2_set(.7f, .9f)), thickness, fill_solid, primary_color, op_union);
            renderer_end_combination(gfx_context, false);
            break;
        }

    case ICON_SCALE:
        {
            vec2 box_min = aabb_bilinear(&box, vec2_set(.1f, .5f));
            vec2 box_max = aabb_bilinear(&box, vec2_set(.5f, .9f));
            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_box(gfx_context, box_min.x, box_min.y, box_max.x, box_max.y, secondary_color);
            renderer_draw_double_arrow(gfx_context, aabb_bilinear(&box, vec2_set(.55f, .45f)), aabb_bilinear(&box, vec2_set(.9f, .1f)),
                               0.05f * max_radius, 0.2f, primary_color);
            renderer_end_combination(gfx_context, false);
            break;
        }
    default: break;
    }
}

