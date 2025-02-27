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
    float max_radius = vec2_length(extent);

    aabb unit_box = (aabb) {.min = vec2_splat(-1.f), vec2_splat(1.f)};
    float loop_1s = sl_loop(time_in_second, 1.f);
    float loop_2s = sl_loop(time_in_second, 2.f);
    float loop_5s = sl_loop(time_in_second, 5.f);

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
            float radius = sl_wave_base(loop_2s, .2f, .2f) * max_radius;
            renderer_draw_disc(gfx_context, center, radius, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_ORIENTEDBOX:
        {
            vec2 p0 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(.7f, 0.f), vec2_angle(time_in_second)));
            vec2 p1 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(-.7f, 0.f), vec2_angle(time_in_second)));
            float width = sl_wave_base(loop_2s, .2f, .3f) * max_radius;
            renderer_draw_orientedbox(gfx_context, p0, p1, width, 0.f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_ELLIPSE:
        {
            vec2 p0 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(-.5f, -.5f), vec2_angle(-time_in_second)));
            vec2 p1 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(.5f, .5f), vec2_angle(-time_in_second)));
            float width = sl_wave_base(loop_2s, .2f, .3f) * max_radius;
            renderer_draw_ellipse(gfx_context, p0, p1, width, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_PIE:
        {
            vec2 point = vec2_scale(vec2_angle(time_in_second * 1.3f), sl_wave_base(loop_5s, 0.4f, 0.2f));
            renderer_draw_pie(gfx_context, center, aabb_project(&unit_box, &box, point), sl_wave_base(loop_2s, VEC2_PI_4, VEC2_PI_2),
                              0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_ARC:
        {
            renderer_draw_arc(gfx_context, center, vec2_angle(-time_in_second), sl_wave_base(loop_2s + VEC2_PI, VEC2_PI_2, VEC2_PI_4),
                             max_radius * 0.40f, max_radius * 0.1f, fill_solid,primary_color, op_add);
            break;
        }
    case ICON_TRIANGLE:
        {
            vec2 p0 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(.6f, .6f), vec2_angle(-time_in_second)));
            vec2 p1 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(-.2f, -.5f), vec2_angle(-time_in_second)));
            vec2 p2 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(-.4f, .3f), vec2_angle(-time_in_second)));
            renderer_draw_triangle(gfx_context, p0, p1, p2, 0.f, 0.f, fill_solid, primary_color, op_add);
            break;
        }
    case ICON_CAPSULE:
        {
            vec2 p0 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(.5f, 0.f), vec2_angle(time_in_second)));
            vec2 p1 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(-.5f, 0.f), vec2_angle(time_in_second)));
            float radius0 = sl_wave_base(loop_2s, .3f, -.2f) * max_radius;
            float radius1 = sl_wave_base(loop_5s, .3f, -.1f) * max_radius;
            renderer_draw_unevencapsule(gfx_context, p0, p1, radius0, radius1, 0.f, fill_solid, primary_color, op_add);
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
            float t = sl_wave_base(loop_2s, 0.2f, 0.8f);
            vec2 box_min = aabb_bilinear(&box, vec2_set(.1f, .5f - (1.f - t) * .2f));
            vec2 box_max = aabb_bilinear(&box, vec2_set(.5f + (1.f - t) * .2f, .9f));
            vec2 p0 = vec2_lerp( vec2_set(.5f, -.5f), vec2_set(0.9f, -0.9f), t);
            vec2 p1 = vec2_lerp( vec2_set(.5f, -.5f), vec2_set(.1f, -.1f), t);
            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_box(gfx_context, box_min.x, box_min.y, box_max.x, box_max.y, secondary_color);
            renderer_draw_double_arrow(gfx_context, aabb_project(&unit_box, &box, p0), aabb_project(&unit_box, &box, p1),
                               0.025f * max_radius, 0.2f, primary_color);
            renderer_end_combination(gfx_context, false);
            break;
        }
    case ICON_ROTATE:
        {
            vec2 rotation = vec2_angle(time_in_second);
            vec2 p0 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(0.f, -.4f), rotation));
            vec2 p1 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(0.f, .4f), rotation));
            renderer_draw_orientedbox(gfx_context, p0, p1, extent.x * .8f, 0.f, 0.f, fill_solid, secondary_color, op_add);

            rotation = vec2_angle(-sl_wave_base(loop_2s, -VEC2_PI_4, VEC2_PI_2));
            p0 = aabb_project(&unit_box, &box, vec2_set(-0.8f, 0.f));
            p1 = aabb_project(&unit_box, &box, vec2_set(0.f, -.8f));
            vec2 p2 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(0.8f, .0f), rotation));

            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_arc_from_circle(gfx_context, p0, p1, p2, .05f * max_radius, fill_solid, primary_color, op_union);

            rotation = vec2_angle(-sl_wave_base(loop_2s, -VEC2_PI_4 + 0.3f, VEC2_PI_2));
            p0 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(0.55f, .0f), rotation));
            p1 = aabb_project(&unit_box, &box, vec2_rotate(vec2_set(1.0f, .0f), rotation));

            renderer_draw_triangle(gfx_context, p0, p1, p2, 0.f, 0.f, fill_solid, primary_color, op_union);
            renderer_end_combination(gfx_context, false);

            break;
        }
    case ICON_LAYERUP:
        {
            renderer_draw_orientedbox(gfx_context, aabb_project(&unit_box, &box, vec2_set(0.f, .5f)),
                                      aabb_project(&unit_box, &box, vec2_set(.6f, 0.f)),
                                      0.8f * extent.x, 0.f, 0.f, fill_solid, primary_color, op_add);
            
            renderer_draw_orientedbox(gfx_context, aabb_project(&unit_box, &box, vec2_set(0.f, .1f)),
                                      aabb_project(&unit_box, &box, vec2_set(.6f, -.4f)),
                                      0.8f * extent.x, 0.f, 0.f, fill_solid, secondary_color, op_add);
            

            vec2 target = vec2_lerp( vec2_set(-.65f, 0.f), vec2_set(-0.65f, -.8f), sl_impulse(sl_wave(loop_1s)));

            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_arrow_filled(gfx_context, aabb_project(&unit_box, &box, vec2_set(-0.65f, .8f)),
                                      aabb_project(&unit_box, &box, target), 0.025f * extent.x, .15f, primary_color);
            renderer_end_combination(gfx_context, false);

            break;
        }
    case ICON_LAYERDOWN:
        {
            renderer_draw_orientedbox(gfx_context, aabb_project(&unit_box, &box, vec2_set(-0.6f, .5f)),
                                      aabb_project(&unit_box, &box, vec2_set(0.f, 0.f)),
                                      0.8f * extent.x, 0.f, 0.f, fill_solid, secondary_color, op_add);

            renderer_draw_orientedbox(gfx_context, aabb_project(&unit_box, &box, vec2_set(-.6f, .1f)),
                                      aabb_project(&unit_box, &box, vec2_set(0.f, -.4f)),
                                      0.8f * extent.x, 0.f, 0.f, fill_solid, primary_color, op_add);

            vec2 target = vec2_lerp( vec2_set(.65f, 0.f), vec2_set(.65f, .8f), sl_impulse(sl_wave(loop_1s)));

            renderer_begin_combination(gfx_context, 1.f);
            renderer_draw_arrow_filled(gfx_context, aabb_project(&unit_box, &box, vec2_set(0.65f, -.8f)),
                                    aabb_project(&unit_box, &box, target), 0.025f * extent.x, .15f, primary_color);
            renderer_end_combination(gfx_context, false);

            break;
        }
    default: break;
    }
}

