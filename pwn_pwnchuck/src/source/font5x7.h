#ifndef PWNCHUCK_FONT5X7_H
#define PWNCHUCK_FONT5X7_H
#include "types.h"
#define FONT_W 5
#define FONT_H 7
#define FONT_FIRST 32
#define FONT_LAST 126
extern const u8 font_Monaco[FONT_LAST - FONT_FIRST + 1][FONT_H];
extern const u8 font_Consola[FONT_LAST - FONT_FIRST + 1][FONT_H];

#endif
