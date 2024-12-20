#ifndef __BIARC_H__
#define __BIARC_H__


#include "arc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool biarc_generate(vec2 p0, vec2 p1, vec2 p2, struct arc* arcs);

#ifdef __cplusplus
}
#endif


#endif