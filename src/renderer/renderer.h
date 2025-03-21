#ifndef __RENDERER__H__
#define __RENDERER__H__

#include <stdint.h>
#include <stddef.h>
#include "../system/aabb.h"
#include "../shaders/common.h"
#include "../shaders/font.h"


struct renderer;
struct mu_Context;
struct view_proj;

#ifdef __cplusplus
extern "C" {
#endif

struct renderer* renderer_init(void* device, uint32_t width, uint32_t height);
void renderer_resize(struct renderer* r, uint32_t width, uint32_t height);
void renderer_reload_shaders(struct renderer* r);
void renderer_begin_frame(struct renderer* r);
void renderer_flush(struct renderer* r, void* drawable);
void renderer_debug_interface(struct renderer* r, struct mu_Context* gui_context);
void renderer_end_frame(struct renderer* r);
void renderer_terminate(struct renderer* r);

void renderer_set_clear_color(struct renderer* r, draw_color color);
void renderer_set_cliprect(struct renderer* r, uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y);
void renderer_set_cliprect_relative(struct renderer * r, aabb const* box);
void renderer_set_culling_debug(struct renderer* r, bool b);
void renderer_set_viewproj(struct renderer* r, const struct view_proj* vp);

void renderer_begin_combination(struct renderer* r, float smooth_value);
void renderer_end_combination(struct renderer* r, bool outline);

// note : it is assumed that all colors are in sRGB and will be converted in the shader
void renderer_draw_disc(struct renderer* r, vec2 center, float radius, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_orientedbox(struct renderer* r, vec2 p0, vec2 p1, float width, float roundness, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_line(struct renderer* r, vec2 p0, vec2 p1, float width, draw_color color, enum sdf_operator op);
void renderer_draw_arrow(struct renderer* r, vec2 p0, vec2 p1, float width, draw_color color);
void renderer_draw_arrow_solid(struct renderer* r, vec2 p0, vec2 p1, float width, draw_color color);
void renderer_draw_doublearrow(struct renderer* r, vec2 p0, vec2 p1, float width, draw_color color);
void renderer_draw_doublearrow_solid(struct renderer* r, vec2 p0, vec2 p1, float width, draw_color color);
void renderer_draw_ellipse(struct renderer* r, vec2 p0, vec2 p1, float width, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_triangle(struct renderer* r, vec2 p0, vec2 p1, vec2 p2, float roundness, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_pie(struct renderer* r, vec2 center, vec2 point, float aperture, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_unevencapsule(struct renderer* r, vec2 p0, vec2 p1, float radius0, float radius1, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_arc(struct renderer* r, vec2 center, vec2 direction, float aperture, float radius, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_arc_from_circle(struct renderer* r, vec2 p0, vec2 p1, vec2 p2, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_trapezoid(struct renderer* r, vec2 p0, vec2 p1, float radius0, float radius1, float roundness, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op);
void renderer_draw_box(struct renderer* r, float x0, float y0, float x1, float y1, draw_color color);
void renderer_draw_aabb(struct renderer* r, aabb box, draw_color color);
void renderer_draw_char(struct renderer* r, float x, float y, char c, draw_color color);
void renderer_draw_text(struct renderer* r, float x, float y, const char* text, draw_color color);

#ifdef __cplusplus
}
#endif

#endif

