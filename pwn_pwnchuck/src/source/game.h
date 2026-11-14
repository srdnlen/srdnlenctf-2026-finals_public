#ifndef PWNCHUCK_GAME_H
#define PWNCHUCK_GAME_H

#include <stdbool.h>
#include "input.h"

typedef enum {
	ITEM_NONE = 0,
	ITEM_HINT,
	ITEM_TORCH,
} ItemKind;

typedef struct {
	int px, py;           // room local position, world units ?? unsigned??
	s32 vx_q8;            // velocity, 8.8 units per frame
	s32 vy_q8;            // velocity, 8.8 units per frame
	s16 rem_x, rem_y;     // sub-unit carry, so slow movement still moves
	s16 room;             // signed: the table is one based, see rooms.h
	s16 prev_room;             // signed: the table is one based, see rooms.h

	u16 remaining_frames; // Kong's Maze clock; only the maze reads it
} Player;

void game_init(void);
void game_update(const InputState *in);
void game_draw(void);

// the debug overlay only exists in development builds
#ifdef PWNCHUCK_DEV
void *game_draw_debug(void *args);
void game_toggle_debug(void);
bool game_debug_enabled(void);
#endif

#endif
