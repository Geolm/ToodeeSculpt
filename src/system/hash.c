#include "hash.h"

//-----------------------------------------------------------------------------
uint32_t hash_fnv_1a(const void *data, size_t length)
{
    uint32_t hash = 0x811c9dc5;
    uint8_t* p = (uint8_t*) data;

    for(size_t i=0; i<length; ++i)
        hash = (hash ^ *p++) * 0x01000193;

    return hash;
}

//-----------------------------------------------------------------------------
uint32_t hash_jenkins(const void* data, size_t length)
{
    uint8_t* p = (uint8_t*) data;
    size_t i = 0;
    uint32_t hash = 0;
    while (i != length) 
    {
        hash += p[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
}