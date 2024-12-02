#ifndef __HASH__H__
#define __HASH__H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 32bit fnv-1a hash
uint32_t hash_fnv_1a(const void *data, size_t length);

// Jenkins's one_at_a_time hash 
uint32_t hash_jenkins(const void* data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
