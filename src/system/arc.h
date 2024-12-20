#ifndef __ARC_H__
#define __ARC_H__

#include "aabb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct arc
{
    vec2 center;
    vec2 direction;
    float aperture;
    float radius;
};

// compute a circle out of 3 points
// radius is negative is the points are colinear
void circle_from_points(vec2 p0, vec2 p1, vec2 p2, vec2* center, float* radius);

// radius is negative is the points are colinear
void arc_from_points(vec2 p0, vec2 p1, vec2 p2, vec2* center, vec2* direction, float* aperture, float* radius);

aabb aabb_from_arc(vec2 center, vec2 direction, float radius, float aperture);
aabb aabb_from_pie(vec2 center, vec2 direction, float radius, float aperture);



#ifdef __cplusplus
}
#endif

#endif