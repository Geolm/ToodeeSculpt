#ifndef __BIARC_H__
#define __BIARC_H__


#include "arc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// outputs the control points of the quadratic bezier curve that goes though all points (p0, p1, p2)
void bezier_from_path(vec2 p0, vec2 p1, vec2 p2, vec2* output);

// outputs two arcs from a quadratic bezier curve that goes through (p0, p1, p2)
// can output an arc with negative radius if the points are colinear
void biarc_from_path(vec2 p0, vec2 p1, vec2 p2, struct arc* arcs);

// outputs two arcs that approximate the quadratic bezier curves defined by the control points (c0, c1, c2)
// can output an arc with negative radius if the points are colinear
void biarc_from_bezier(vec2 c0, vec2 c1, vec2 c2, struct arc* arcs);

// "tessellate" a quadratic bezier curve that goes through (p0, p1, p2) with multiple arcs
// 
//      [arcs]          the array should be enough to hold the "worst case" which is (1<<max_subdivision)
//                      the arcs with minor radius are invalid (colinear points or other errors)
//
//      [max_error]     the threshold in units (pixels or whatever) that will trigger a subdivision if the arc is away for the real curve
void biarc_tessellate(vec2 p0, vec2 p1, vec2 p2, uint32_t max_subdivision, float max_error, struct arc* arcs, uint32_t *num_arcs);


void biarc_from_points_tangents(vec2 p0, vec2 p1, float theta0, float theta1, struct arc* arcs);

#ifdef __cplusplus
}
#endif


#endif