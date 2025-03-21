#ifndef __ORTHO__H__
#define __ORTHO__H__

#include "aabb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct view_proj
{
    vec2 projection_scale;
    vec2 projection_translation;   
    vec2 half_window_size;
};

void ortho_set_target(struct view_proj* output, const aabb* target, vec2 window_size);
void ortho_set_viewport(struct view_proj* output, vec2 window_size, vec2 viewport_size, vec2 camera_pos);
void ortho_set_corners(struct view_proj* output, float left, float right, float bottom, float top);
void ortho_set_window_size(struct view_proj* output, vec2 window_size);
vec2 ortho_to_screen_space(struct view_proj const* vp, vec2 world_space);
vec2 ortho_to_world_space(struct view_proj const* vp, vec2 screen_space);
static float ortho_get_radius_scale(struct view_proj const* vp);

//-----------------------------------------------------------------------------------------------------------------------------
static inline float ortho_get_radius_scale(struct view_proj const* vp)
{
    vec2 ortho_scale = vec2_mul(vp->projection_scale, vp->half_window_size);
    return float_max(ortho_scale.x, ortho_scale.y);
}

#ifdef __cplusplus
}
#endif

#endif