#include "sprite.h"
#include "gx2d.h"

int sprite_w(const Sprite *s, int scale){
	return s->w * scale;
}

int sprite_h(const Sprite *s, int scale){
	return s->h * scale;
}

void sprite_draw(const Sprite *s, int x, int y, int scale, const u8 pal[][3]){
	u8 row, col;
	bool has_special = false;
	//check if the sprite has only numbers...
	for(int i = 0; i< s->h; i++){
		const u8 *line = s->rows[i];
		for (int j = 0; j < s->w; j++){
			if (line[j]<='0' || line[j]>='9') if(line[j] != '.')  has_special = true;
		}
	}

	for (row = 0; row < s->h; row++){
		const u8 *line = s->rows[row];

		for (col = 0; col < s->w; col++){
			u8 px = line[col];
			if((has_special && px != '\x00') || ((!has_special ) && px != '.' )){
				const u8 *colour;
				if(has_special) colour = pal[px];
				else colour = pal[px - '1'];
				gx2d_quad(x + col * scale, y + row * scale, scale, scale, colour[0], colour[1], colour[2], 0xff);
			}
		}
	}
}

void sprite_draw_centered(const Sprite *s, int cx, int cy, int scale,
			  const u8 pal[][3]){
	sprite_draw(s, cx - sprite_w(s, scale) / 2, cy - sprite_h(s, scale) / 2,
		    scale, pal);
}
