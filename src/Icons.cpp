#include "Icons.h"
#include "system/stateless.h"
#include "system/vec2.h"


void DrawIcon(Renderer& renderer, aabb box, icon_type icon, draw_color primaray_color, draw_color secondary_color, float time_in_second)
{
    vec2 center = vec2_scale(box.min + box.max, .5f);
    vec2 extent = vec2_scale(box.max - box.min, .5f);
    float max_radius = fminf(extent.x, extent.y);

    switch(icon)
    {
    case ICON_CLOSE:    // a disc with a cross
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .5f);
            renderer.BeginCombination(1.f);
            renderer.DrawCircleFilled(center, max_radius * .8f, primaray_color);
            renderer.DrawOrientedBox(safe_box.min, safe_box.max, max_radius * .1f, 0.f, secondary_color);
            renderer.DrawOrientedBox(aabb_get_vertex(&safe_box, aabb_bottom_left), aabb_get_vertex(&safe_box, aabb_top_right),
                                     max_radius * .1f, 0.f, secondary_color);
            renderer.EndCombination();
            break;
        }
    case ICON_COLLAPSED:    // a triangle pointing to the right
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .4f);
            renderer.DrawTriangleFilled(aabb_get_vertex(&safe_box, aabb_bottom_left), 
                                        aabb_get_vertex(&safe_box, aabb_top_left), center, 0.f, primaray_color);
            break;
        }
    case ICON_EXPANDED:     // a triangle pointing to the bottom
        {
            aabb safe_box = box;
            aabb_scale(&safe_box, .4f);
            renderer.DrawTriangleFilled(aabb_get_vertex(&safe_box, aabb_top_left), 
                                        aabb_get_vertex(&safe_box, aabb_top_right), center, 0.f, primaray_color);
            break;
        }
    case ICON_CHECK:
        {
            vec2 a = vec2_add(center, vec2_scale((vec2){ 0.7f, -0.6f}, max_radius));
            vec2 b = vec2_add(center, vec2_scale((vec2){-0.2f,  0.5f}, max_radius));
            vec2 c = vec2_add(center, vec2_scale((vec2){-0.65f, 0.0f}, max_radius));
            renderer.BeginCombination(1.f);
            renderer.DrawOrientedBox(a, b, 0.f, max_radius * .1f, primaray_color);
            renderer.DrawOrientedBox(b, c, 0.f, max_radius * .1f, primaray_color);
            renderer.EndCombination();
            break;
        }
    default: break;
    }

}
