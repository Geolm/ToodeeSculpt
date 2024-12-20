#ifndef __BIARC_H__
#define __BIARC_H__


#include "arc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// outputs two arcs from a quadratic bezier curve that goes through (p0, p1, p2)
// outputs an arc with negative radius if the points are colinear
void biarc_generate(vec2 p0, vec2 p1, vec2 p2, struct arc* arcs);

// "tessellate" a quadratic bezier curve that goes through (p0, p1, p2) with multiple arcs
// returns NULL if the points are colinear
// 
// arcs array should be enough to hold the "worst case" which is 1<<max_subdivision
void biarc_tessellate(vec2 p0, vec2 p1, vec2 p2, uint32_t max_subdivision, struct arc* arcs, uint32_t *num_arcs);

#ifdef __cplusplus
}
#endif


#endif