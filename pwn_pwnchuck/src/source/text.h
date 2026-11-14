#ifndef PWNCHUCK_TEXT_H
#define PWNCHUCK_TEXT_H

#include "types.h"
#include <string.h>
#include "font5x7.h"
#include "gx2d.h"
/*
 * Text is drawn with the generated 5x7 font, one quad per lit pixel.  All
 * coordinates are in the 640x480 logical screen space used by gx2d.
 */
typedef enum { MAIN_FONT, DEBUG_FONT } FontCoice;



void text_draw(s16 x, s16 y, u8 scale, u8 r, u8 g, u8 b, const char *s ,FontCoice font);
void text_center(s16 cx, s16 y, u8 scale, u8 r, u8 g, u8 b, const char *s,FontCoice font);

/* Greedy word wrap; max_chars is the per line budget at the given scale. */
void text_wrap(s16 x, s16 y, u8 scale, u8 r, u8 g, u8 b, u8 max_chars, const char *s,FontCoice font);

s16 text_width(const char *s, u8 scale);

#endif
