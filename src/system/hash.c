#include "hash.h"


void hash_fnv_1a(const void *data, size_t size, uint32_t* hash)
{
    *hash = 0x811c9dc5;
    uint8_t* p = (uint8_t*) data;

    for(size_t i=0; i<size; ++i)
    {
        *hash = (*hash ^ *p++) * 0x01000193;
    }
}