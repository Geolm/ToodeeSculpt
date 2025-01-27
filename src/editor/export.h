#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>
#include "../system/format.h"

struct primitive;

#ifdef __cplusplus
extern "C" {
#endif

void copy_boiler_plate(struct string_buffer* b);
void primitive_export_shadertoy(struct string_buffer* b, struct primitive * const p, uint32_t index, float smooth_value);
void finish_shadertoy(struct string_buffer* b);


#ifdef __cplusplus
}
#endif

#endif