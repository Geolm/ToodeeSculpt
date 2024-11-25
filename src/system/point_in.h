#ifndef _POINT_IN_H_
#define _POINT_IN_H_

#include "vec2.h"

#ifdef __cplusplus
extern "C" {
#endif

bool point_in_triangle(vec2 p0, vec2 p1, vec2 p2, vec2 point);
bool point_in_oriented_box(vec2 p0, vec2 p1, float width, vec2 point);
bool point_in_ellipse(vec2 p0, vec2 p1, float width, vec2 point);

static inline bool point_in_disc(vec2 center, float radius, vec2 point)
{
    return (vec2_sq_distance(center, point) <= radius * radius);
}

#ifdef __cplusplus
}
#endif

#endif
