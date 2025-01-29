#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <stdint.h>
#include "../system/format.h"

struct primitive;

#ifdef __cplusplus
extern "C" {
#endif

void shadertoy_start(struct string_buffer* b);
void shadertoy_export_primitive(struct string_buffer* b, struct primitive * const p, uint32_t index, float smooth_value);
void shadertoy_finalize(struct string_buffer* b);


#ifdef __cplusplus
}
#endif

#endif