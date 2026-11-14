#ifndef PWNCHUCK_SPRITE_H
#define PWNCHUCK_SPRITE_H

#include "types.h"

/*
 * Sprites are authored as text rows: '.' is transparent and '1'..'6' pick a
 * colour out of the palette handed to sprite_draw().
 */
typedef struct {
	const char *const *rows;
	u8 w, h;
} Sprite;

void sprite_draw(const Sprite *s, int x, int y, int scale, const u8 pal[][3]);
void sprite_draw_centered(const Sprite *s, int cx, int cy, int scale,
			  const u8 pal[][3]);
int sprite_w(const Sprite *s, int scale);
int sprite_h(const Sprite *s, int scale);

#endif
