#ifndef __BIARC_H__
#define __BIARC_H__


#include "arc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BIARC_SPLINE_MAX_POINT (256)

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


// given 2 points and two tangents output a biarc
void biarc_from_points_tangents(vec2 p0, vec2 p1, float angle0, float angle1, struct arc* arcs);

// based on Interpolating Splines of Biarcs from a Sequence of Planar Points
//
// generate a spline of biarcs that goes through all input points and had G1 continuity
//
//      [arcs]          the array should at least be (num_points-1) * 2 sized
//
//  returns the number of arcs generated
uint32_t biarc_spline(vec2 const* points, uint32_t num_points, struct arc* arcs);

// returns the guess tangent for the point at [index], tangent is simply based on neighbors points and distance
float initial_tangent_guess(const vec2* points, uint32_t num_points, uint32_t index);

#ifdef __cplusplus
}
#endif


#endif