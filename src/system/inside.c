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

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

//-----------------------------------------------------------------------------
bool point_in_disc(vec2 center, float radius, vec2 point)
{
    return (vec2_sq_distance(center, point) <= radius * radius);
}
