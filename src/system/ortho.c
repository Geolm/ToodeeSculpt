#include "ortho.h"


//-----------------------------------------------------------------------------------------------------------------------------
void ortho_set_viewport(struct view_proj* output, vec2 window_size, vec2 viewport_size, vec2 camera_pos)
{
    aabb target =
    {
        .min = camera_pos,
        .max = vec2_add(viewport_size, camera_pos)
    };
    ortho_set_target(output, &target, window_size);
}

//-----------------------------------------------------------------------------------------------------------------------------
void ortho_set_target(struct view_proj* output, const aabb* target, vec2 window_size)
{
    float left = target->min.x;
    float right = target->max.x;
    float bottom = target->min.y;
    float top = target->max.y;
    
    float window_ratio = window_size.x / window_size.y;
    vec2 viewport_size = aabb_get_size(target);
    float viewport_ratio = viewport_size.x / viewport_size.y;

    if (window_ratio>1.f)
    {
        float width = (viewport_size.x * window_ratio) / viewport_ratio;
        float bias = (width - viewport_size.x) * .5f;
        left -= bias;
        right += bias;	
    }
    else
    {
        float height = (viewport_size.y * viewport_ratio)/ window_ratio;
        float bias = (height - viewport_size.y) * .5f;
        top += bias;
        bottom -= bias;
    }

    ortho_set_corners(output, left, right, bottom, top);
    ortho_set_window_size(output, window_size);
}

//-----------------------------------------------------------------------------------------------------------------------------
void ortho_set_corners(struct view_proj* output, float left, float right, float bottom, float top)
{
    output->projection_scale = vec2_set(2.f / (right-left), 2.f / (top-bottom));
    output->projection_translation = vec2_set(-((left+right) / (right-left)), -((bottom+top) / (top-bottom)));
}

//-----------------------------------------------------------------------------------------------------------------------------
void ortho_set_window_size(struct view_proj* output, vec2 window_size)
{
    output->half_window_size = vec2_scale(window_size, .5f);
}

//-----------------------------------------------------------------------------------------------------------------------------
vec2 ortho_to_screen_space(struct view_proj const* vp, vec2 world_space)
{
    vec2 clip_space = vec2_add(vec2_mul(world_space, vp->projection_scale), vp->projection_translation);
    vec2 screen_space = vec2_mul(vec2_add(clip_space, vec2_splat(1.f)), vp->half_window_size);
    return screen_space;
}

//-----------------------------------------------------------------------------------------------------------------------------
vec2 ortho_to_world_space(struct view_proj const* vp, vec2 screen_space)
{
    vec2 clip_space = vec2_sub(vec2_div(screen_space, vp->half_window_size), vec2_one());
    vec2 world_space = vec2_div(vec2_sub(clip_space, vp->projection_translation), vp->projection_scale);
    return world_space;
}

