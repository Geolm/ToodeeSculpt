#include "Icons.h"
#include "system/stateless.h"


void DrawIcon(Renderer& renderer, aabb box, icon_type icon, draw_color primaray_color, draw_color secondary_color, float time_in_second)
{
    vec2 center = vec2_scale(box.min + box.max, .5f);
    vec2 extent = vec2_scale(box.max - box.min, .5f);
    float max_radius = fminf(extent.x, extent.y);
    aabb safe_box = box;
    aabb_scale(&safe_box, .8f);

    switch(icon)
    {
    case ICON_CLOSE:
        {
            aabb_scale(&safe_box, .6f);
            renderer.BeginCombination(1.f);
            renderer.DrawCircleFilled(center, max_radius * .8f, primaray_color);
            renderer.DrawOrientedBox(safe_box.min, safe_box.max, max_radius * .1f, 0.f, secondary_color);
            renderer.DrawOrientedBox(aabb_get_vertex(&safe_box, aabb_bottom_left), aabb_get_vertex(&safe_box, aabb_top_right),
                                     max_radius * .1f, 0.f, secondary_color);
            renderer.EndCombination();
            break;
        }
    }

}
