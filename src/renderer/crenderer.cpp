#include "crenderer.h"
#include "Renderer.h"

// just redirect the call to c++ class

void renderer_drawcircle(void* cpp_renderer, vec2 center, float radius, float thickness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawCircle(center, radius, thickness, color, op);
}

void renderer_drawcircle_filled(void* cpp_renderer, vec2 center, float radius, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawCircleFilled(center, radius, color, op);
}

void renderer_draworientedbox(void* cpp_renderer, vec2 p0, vec2 p1, float width, float thickness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawOrientedBox(p0, p1, width, thickness, color, op);
}

void renderer_draworientedbox_filled(void* cpp_renderer, vec2 p0, vec2 p1, float width, float roundness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawOrientedBoxFilled(p0, p1, width, roundness, color, op);
}

void renderer_drawellipse(void* cpp_renderer, vec2 p0, vec2 p1, float width, float thickness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawEllipse(p0, p1, width, thickness, color, op);
}

void renderer_drawellipse_filled(void* cpp_renderer, vec2 p0, vec2 p1, float width, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawEllipseFilled(p0, p1, width, color, op);
}

void renderer_drawtriangle(void* cpp_renderer, vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawTriangle(p0, p1, p2, thickness, color, op);
}

void renderer_drawtriangle_filled(void* cpp_renderer, vec2 p0, vec2 p1, vec2 p2, float roundness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawTriangleFilled(p0, p1, p2, roundness, color, op);
}

void renderer_drawpie(void* cpp_renderer, vec2 center, vec2 point, float aperture, float width, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawPie(center, point, aperture, width, color, op);
}

void renderer_drawpie_filled(void* cpp_renderer, vec2 center, vec2 point, float aperture, draw_color color, enum sdf_operator op )
{
    ((Renderer*) cpp_renderer)->DrawPieFilled(center, point, aperture, color, op);
}

void renderer_drawring(void* cpp_renderer, vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawRing(p0, p1, p2, thickness, color, op);
}

void renderer_drawring_filled(void* cpp_renderer, vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, enum sdf_operator op)
{
    ((Renderer*) cpp_renderer)->DrawRingFilled(p0, p1, p2, thickness, color, op);
}
