#include "ortho.h"


//-----------------------------------------------------------------------------
void ortho_set_viewport(struct view_proj* output, vec2 window_size, vec2 viewport_size)
{
    float window_ratio = window_size.x / window_size.y;
    float viewport_ratio = viewport_size.x / viewport_size.y;
    float left, right, bottom, top;

    if (window_size.x > window_size.y)
    {
        float width = viewport_size.x / (viewport_ratio/window_ratio);
        float bias = (width - viewport_size.x) / 2.f;	

        left = -bias;
        right = viewport_size.x + bias;
        bottom = 0.f;
        top = viewport_size.y;
    }
    else
    {
        float height = viewport_size.y / (window_ratio/viewport_ratio);
        float bias = (height - viewport_size.y) / 2.f;

        left = 0.f;
        right = viewport_size.x;
        bottom = -bias;
        top = viewport_size.y + bias;
    }

    output->projection_scale = vec2_set(2.f / (right-left), 2.f / (top-bottom));
    output->projection_translation = vec2_set(-((left+right) / (right-left)), -((bottom+top) / (top-bottom)));
    output->window_size = window_size;
}

//-----------------------------------------------------------------------------
vec2 ortho_transform(struct view_proj const* vp, vec2 camera_pos, float scale, vec2 world_space)
{
    vec2 view_space = vec2_scale(vec2_sub(world_space, camera_pos), scale);
    vec2 clip_space = vec2_add(vec2_mul(view_space, vp->projection_scale), vp->projection_translation);
    vec2 screen_space = vec2_mul(vec2_add(clip_space, vec2_splat(1.f)), vec2_scale(vp->window_size, .5f));
    return screen_space;
}

