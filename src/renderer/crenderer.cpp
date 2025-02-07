#include "crenderer.h"
#include "Renderer.h"

// just redirect the call to c++ class

void renderer_begin_combination(void* cpp_renderer, float smooth_value)
{
    ((Renderer*) cpp_renderer)->BeginCombination(smooth_value);
}

void renderer_end_combination(void* cpp_renderer)
{
    ((Renderer*) cpp_renderer)->EndCombination();
}

void renderer_drawbox(void* cpp_renderer, aabb box, draw_color color)
{
    ((Renderer*) cpp_renderer)->DrawBox(box, color);
}

void renderer_drawdisc(void* cpp_renderer, vec2 center, float radius, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawDisc(center, radius, thickness, fillmode, color, op);
}

void renderer_draworientedbox(void* cpp_renderer, vec2 p0, vec2 p1, float width, float roundness, float thickness,
                              enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawOrientedBox(p0, p1, width, roundness, thickness, fillmode, color, op);
}

void renderer_drawline(void* cpp_renderer, vec2 p0, vec2 p1, float width, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawOrientedBox(p0, p1, width, 0.f, 0.f, fill_solid, color, op);
}

void renderer_drawellipse(void* cpp_renderer,vec2 p0, vec2 p1, float width, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawEllipse(p0, p1, width, thickness, fillmode, color, op);
}

void renderer_drawtriangle(void* cpp_renderer, vec2 p0, vec2 p1, vec2 p2, float roundness, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawTriangle(p0, p1, p2, roundness, thickness, fillmode, color, op);
}

void renderer_drawpie(void* cpp_renderer,vec2 center, vec2 point, float aperture, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawPie(center, point, aperture, thickness, fillmode, color, op);
}

void renderer_drawring_frompoints(void* cpp_renderer,vec2 p0, vec2 p1, vec2 p2, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawRing(p0, p1, p2, thickness, fillmode, color, op);
}

void renderer_drawring(void* cpp_renderer, vec2 center, vec2 direction, float aperture, float radius, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawRing(center, direction, aperture, radius, thickness, fillmode, color, op);
}

void renderer_drawunevencapsule(void* cpp_renderer, vec2 p0, vec2 p1, float r0, float r1, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawUnevenCapsule(p0, p1, r0, r1, thickness, fillmode, color, op);
}

