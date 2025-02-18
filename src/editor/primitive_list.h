#ifndef __PRIMITIVE_LIST_H__
#define __PRIMITIVE_LIST_H__

#include <stdint.h>
#include "../system/serializer.h"
#include "../system/aabb.h"

struct primitive;

#ifdef __cplusplus
extern "C" {
#endif

void plist_init(uint32_t reservation);
struct primitive* plist_get(uint32_t index);
void plist_clear(void);
uint32_t plist_last(void);
uint32_t plist_size(void);
void plist_push(struct primitive* p);
void plist_erase(uint32_t index);
void plist_insert(uint32_t index, struct primitive* p);
void plist_resize(uint32_t new_size);
void plist_serialize(serializer_context* context, bool normalization, const aabb* edition_zone);
void plist_deserialize(serializer_context* context, uint16_t major, uint16_t minor, bool normalization, const aabb* edition_zone);
void plist_terminate(void);


#ifdef __cplusplus
}
#endif



#endif