#include "icons.h"
#include "system/stateless.h"
#include "system/vec2.h"

#define UNUSED_VARIABLE(a) (void)(a)

void DrawIcon(void* renderer, aabb box, enum icon_type icon, draw_color primaray_color, draw_color secondary_color, float time_in_second)
{
    UNUSED_VARIABLE(time_in_second);
    vec2 center = vec2_scale(vec2_add(box.min, box.max), .5f);
    vec2 extent = vec2_scale(vec2_sub(box.max, box.min), .5f);
    float max_radius = fminf(extent.x, extent.y);

    switch(icon)
    {
    case ICON_CLOSE:    // a disc with a cross
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .5f);
            renderer_begin_combination(renderer, 1.f);
            renderer_drawdisc(renderer, center, max_radius * .8f, -1.f, fill_solid, primaray_color, op_union);
            renderer_draworientedbox(renderer, safe_box.min, safe_box.max, max_radius * .1f, 0.f, 0.f, fill_solid, secondary_color, op_union);
            renderer_draworientedbox(renderer, aabb_get_vertex(&safe_box, aabb_bottom_left), aabb_get_vertex(&safe_box, aabb_top_right),
                                     max_radius * .1f, 0.f, 0.f, fill_solid, secondary_color, op_union);
            renderer_end_combination(renderer);
            break;
        }
    case ICON_COLLAPSED:    // a triangle pointing to the right
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .4f);
            renderer_drawtriangle_filled(renderer, aabb_get_vertex(&safe_box, aabb_bottom_left), 
                                         aabb_get_vertex(&safe_box, aabb_top_left), center, 0.f, primaray_color, op_union);
            break;
        }
    case ICON_EXPANDED:     // a triangle pointing to the bottom
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .4f);
            renderer_drawtriangle_filled(renderer, aabb_get_vertex(&safe_box, aabb_top_left), 
                                         aabb_get_vertex(&safe_box, aabb_top_right), center, 0.f, primaray_color, op_union);
            break;
        }
    case ICON_CHECK:
        {
            vec2 a = vec2_add(center, vec2_scale((vec2){ 0.7f, -0.6f}, max_radius));
            vec2 b = vec2_add(center, vec2_scale((vec2){-0.2f,  0.5f}, max_radius));
            vec2 c = vec2_add(center, vec2_scale((vec2){-0.65f, 0.0f}, max_radius));
            renderer_begin_combination(renderer, 1.f);
            renderer_draworientedbox(renderer, a, b, 0.f, max_radius * .1f, 0.f, fill_solid, primaray_color, op_union);
            renderer_draworientedbox(renderer, b, c, 0.f, max_radius * .1f, 0.f, fill_solid, primaray_color, op_union);
            renderer_end_combination(renderer);
            break;
        }
    default: break;
    }
}

