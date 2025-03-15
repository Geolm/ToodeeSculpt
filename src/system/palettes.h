#ifndef __PALETTES_H
#define __PALETTES_H

#include <stdint.h>
#include <stdbool.h>
#include "serializer.h"

// https://lospec.com/palette-list/na16
static const uint32_t na16_light_grey = 0xffae8f8c;
static const uint32_t na16_dark_grey = 0xff634558;
static const uint32_t na16_dark_brown = 0xff37213e;
static const uint32_t na16_brown = 0xff48639a;
static const uint32_t na16_light_brown = 0xff7d9bd7;
static const uint32_t na16_light_green = 0xffbaedf5;
static const uint32_t na16_green = 0xff41c7c0;
static const uint32_t na16_dark_green = 0xff347d64;
static const uint32_t na16_orange = 0xff3a94e4; 
static const uint32_t na16_red = 0xff3b309d;
static const uint32_t na16_pink = 0xff7164d2;
static const uint32_t na16_purple = 0xff7f3770;
static const uint32_t na16_light_blue = 0xffc1c47e;
static const uint32_t na16_blue = 0xff9d8534;
static const uint32_t na16_dark_blue = 0xff4b4317;
static const uint32_t na16_black = 0xff1c0e1f;


// https://lospec.com/palette-list/slso8
static const uint32_t slso8_numentries = 8;
static const uint32_t slso8_palette[slso8_numentries] = {0xff452b0d, 0xff563c20, 0xff684e54, 0xff7a698d, 0xff5981d0, 0xff5eaaff, 0xffa3d4ff, 0xffd6ecff};


struct palette
{
    uint32_t* entries;
    uint32_t num_entries;
};

#ifdef __cplusplus
extern "C" {
#endif

void palette_default(struct palette* output);

// returns false on error
bool palette_load_from_hex(const char* filename, struct palette* output);

void palette_serialize(serializer_context* context, struct palette* p);
void palette_deserialize(serializer_context* context, struct palette* p);

void palette_free(struct palette* p);

#ifdef __cplusplus
}
#endif

#endif // __PALETTES_H