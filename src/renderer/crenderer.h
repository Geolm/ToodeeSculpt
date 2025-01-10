#ifndef __CRENDERER__H__
#define __CRENDERER__H__

#include <stdint.h>
#include "../system/aabb.h"
#include "../shaders/common.h"

// this is the C-interface of the renderer
// from now it goes through a ugly cast from a void*

#ifdef __cplusplus
extern "C" {
#endif

void renderer_begin_combination(void* cpp_renderer, float smooth_value);
void renderer_end_combination(void* cpp_renderer);
void renderer_drawbox(void* cpp_renderer, aabb box, draw_color color);
void renderer_drawcircle(void* cpp_renderer, vec2 center, float radius, float thickness, draw_color color, enum sdf_operator op);
void renderer_drawcircle_filled(void* cpp_renderer, vec2 center, float radius, draw_color color, enum sdf_operator op);
void renderer_draworientedbox(void* cpp_renderer, vec2 p0, vec2 p1, float width, float thickness, draw_color color, enum sdf_operator op);
void renderer_draworientedbox_filled(void* cpp_renderer, vec2 p0, vec2 p1, float width, float roundness, draw_color color, enum sdf_operator op);
void renderer_drawellipse(void* cpp_renderer,vec2 p0, vec2 p1, float width, float thickness, draw_color color, enum sdf_operator op);
void renderer_drawellipse_filled(void* cpp_renderer,vec2 p0, vec2 p1, float width, draw_color color, enum sdf_operator op);
void renderer_drawtriangle(void* cpp_renderer,vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, enum sdf_operator op);
void renderer_drawtriangle_filled(void* cpp_renderer,vec2 p0, vec2 p1, vec2 p2, float roundness, draw_color color, enum sdf_operator op);
void renderer_drawpie(void* cpp_renderer,vec2 center, vec2 point, float aperture, float width, draw_color color, enum sdf_operator op);
void renderer_drawpie_filled(void* cpp_renderer,vec2 center, vec2 point, float aperture, draw_color color, enum sdf_operator op );
void renderer_drawring(void* cpp_renderer,vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, enum sdf_operator op);
void renderer_drawring_filled(void* cpp_renderer,vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, enum sdf_operator op);
void renderer_drawarc_filled(void* cpp_renderer, vec2 center, vec2 direction, float aperture, float radius, float thickness, draw_color color, enum sdf_operator op);
void renderer_drawunevencapsule_filled(void* cpp_renderer, vec2 p0, vec2 p1, float r0, float r1, draw_color color, enum sdf_operator op);
void renderer_drawunevencapsule(void* cpp_renderer, vec2 p0, vec2 p1, float r0, float r1, float thickness, draw_color color, enum sdf_operator op);

#ifdef __cplusplus
}
#endif

#endif