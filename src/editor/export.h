#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>

struct primitive;

#ifdef __cplusplus
extern "C" {
#endif

void primitive_export_shadertoy(struct primitive * const p, uint32_t index, float smooth_value);

#ifdef __cplusplus
}
#endif

#endif