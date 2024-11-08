#ifndef _INSIDE_H_
#define _INSIDE_H_

#include "vec2.h"

#ifdef __cplusplus
extern "C" {
#endif

bool point_in_triangle(vec2 p0, vec2 p1, vec2 p2, vec2 point);
bool point_in_disc(vec2 center, float radius, vec2 point);

#ifdef __cplusplus
}
#endif

#endif
