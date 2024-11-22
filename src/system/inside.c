#include "inside.h"

//-----------------------------------------------------------------------------
static inline float edge_sign(vec2 p, vec2 e0, vec2 e1)
{
    return (p.x - e1.x) * (e0.y - e1.y) - (e0.x - e1.x) * (p.y - e1.y);
}

//-----------------------------------------------------------------------------
bool point_in_triangle(vec2 p0, vec2 p1, vec2 p2, vec2 point)
{
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = edge_sign(point, p0, p1);
    d2 = edge_sign(point, p1, p2);
    d3 = edge_sign(point, p2, p0);

    has_neg = (d1 < 0.f) || (d2 < 0.f) || (d3 < 0.f);
    has_pos = (d1 > 0.f) || (d2 > 0.f) || (d3 > 0.f);

    return !(has_neg && has_pos);
}

//-----------------------------------------------------------------------------
bool point_in_oriented_box(vec2 p0, vec2 p1, float width, vec2 point)
{
    vec2 center = vec2_scale(vec2_add(p0, p1), .5f);
    vec2 axis_j = vec2_sub(p1, center);
    float half_height = vec2_normalize(&axis_j);
    vec2 axis_i = vec2_skew(axis_j);
    float half_width = width * .5f;

    point = vec2_sub(point, center);

    if (fabsf(vec2_dot(axis_j, point)) > half_height)
        return false;

    if (fabsf(vec2_dot(axis_i, point)) > half_width)
        return false;

    return true;
}


