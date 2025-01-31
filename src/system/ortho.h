#ifndef __ORTHO__H__
#define __ORTHO__H__

#include "vec2.h"

#ifdef __cplusplus
extern "C" {
#endif

struct view_proj
{
    vec2 projection_scale;
    vec2 projection_translation;   
    vec2 window_size;
};

void ortho_set_viewport(struct view_proj* output, vec2 window_size, vec2 viewport_size);
vec2 ortho_transform_point(struct view_proj const* vp, vec2 camera_pos, float scale, vec2 world_space);
float ortho_get_radius_scale(struct view_proj const* vp, float camera_scale);

#ifdef __cplusplus
}
#endif

#endif