


#include "text.h"

s16 text_width(const char *s, u8 scale){
	return (s16)(strlen(s) * (FONT_W + 1) * scale);
}

void text_draw(s16 x, s16 y, u8 scale, u8 r, u8 g, u8 b, const char *s,FontCoice font)
{
	for (; *s; s++) {
		u8 c = (u8)*s;

		if (c >= FONT_FIRST && c <= FONT_LAST) {
			const u8 (*f)[FONT_H];
			switch (font){
				
				case  MAIN_FONT:
				f = font_Monaco;

				break;
				case DEBUG_FONT:
				    f = font_Consola;
				break;

			}
			const u8 *glyph = f[c - FONT_FIRST];
			u8 row, col;

			for (row = 0; row < FONT_H; row++){
				for (col = 0; col < FONT_W; col++){
					if (glyph[row] & (0x10u >> col)){
						gx2d_quad(x + col * scale,
							  y + row * scale,
							  scale, scale,
							  r, g, b, 0xff);
					}
				}
			}
		}

		x = (s16)(x + (FONT_W + 1) * scale);
	}
}

void text_center(s16 cx, s16 y, u8 scale, u8 r, u8 g, u8 b, const char *s, FontCoice font)
{
	text_draw((s16)(cx - text_width(s, scale) / 2), y, scale, r, g, b, s,font);
}

void text_wrap(s16 x, s16 y, u8 scale, u8 r, u8 g, u8 b, u8 max_chars,
	       const char *s,FontCoice font)
{
	char line[80];
	u8 len = 0;

	if (max_chars >= sizeof(line))
		max_chars = (u8)(sizeof(line) - 1);

	while (*s){
		const char *word = s;
		u8 wlen = 0, i;

		while (*s && *s != ' '){
			s++;
			wlen++;
		}
		while (*s == ' ')
			s++;

		if (len && (u16)len + 1 + wlen > max_chars){
			line[len] = '\0';
			text_draw(x, y, scale, r, g, b, line, font);
			y = (s16)(y + (FONT_H + 2) * scale);
			len = 0;
		}
		if (len)
			line[len++] = ' ';
		for (i = 0; i < wlen && len < max_chars; i++)
			line[len++] = word[i];
	}

	if (len){
		line[len] = '\0';
		text_draw(x, y, scale, r, g, b, line,font);
	}
}
