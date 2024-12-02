#ifndef __HASH__H__
#define __HASH__H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 32bit fnv-1a hash
void hash_fnv_1a(const void *data, size_t size, uint32_t* hash);

#ifdef __cplusplus
}
#endif

#endif
