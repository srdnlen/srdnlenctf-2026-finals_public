#ifndef PWNCHUCK_ART_H
#define PWNCHUCK_ART_H

#include "sprite.h"
enum {
	THEME_DEBUG = 0, /* what the zeroed reserved slot draws with */
	THEME_SMB,
	THEME_DKC,
	THEME_LIB,
	THEME_ZELDA,
	THEME_COUNT,
};
/* the walker: two roll frames */
extern const Sprite art_chuck_a;
extern const Sprite art_chuck_b;

/* pickups */
extern const Sprite art_hint_coin;
extern const Sprite art_torch;

/* furniture */
extern const Sprite art_arrow_left;
extern const Sprite art_arrow_right;

extern const u8 pal_chuck[6][3];
extern const u8 pal_torch[24][3];
extern const u8 pal_hint[6][3];
extern const u8 pal_arrow[6][3];

/*
 * Room colour scheme.  A room picks a theme by number; the renderer takes
 * every flat colour from here, so a new room style is a table entry.
 */
typedef struct {
	u8 sky[3];        /* the backdrop above the ground */
	u8 wall[3];       /* the wall bands */
	u8 wall_line[3];  /* the mortar between the wall bricks */
	u8 ground[3];     /* the ground body */
	u8 ground_top[3]; /* the strip along the ground line */
	u8 panel[3];      /* the status panel */
	u8 text[3];       /* HUD text */
	u8 text_dim[3];   /* message text */
} Theme;

extern const Theme g_themes[THEME_COUNT];

#endif
