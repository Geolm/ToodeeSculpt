/*
 * font9x6.h
 *
 * Created: 4/8/2015 11:32:12 AM
 *  Author: Baron Williams
 */ 

#ifndef FONT9X16_H_
#define FONT9X16_H_

#include <stdint.h>

#define FONT_CHAR_FIRST 32
#define FONT_CHAR_LAST	126
#define FONT_CHARS 95
#define FONT_WIDTH 9
#define FONT_HEIGHT 16
#define FONT_SPACING 1

extern const uint16_t font9x16data[FONT_CHARS][FONT_WIDTH];

#endif /* FONT9X16_H_ */