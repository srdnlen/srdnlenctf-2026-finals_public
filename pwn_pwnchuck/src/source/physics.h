#ifndef PWNCHUCK_PHYSICS_H
#define PWNCHUCK_PHYSICS_H

#include "types.h"
#include "input.h"
#include "game.h"
#define ROOM_W 320
#define ROOM_H 240

#define WALL_MARGIN 8   /* walls are this thick */
#define PLAYER_R 12      /* the walker is round */
#define ENTRY_MARGIN 24 /* how far inside a room you land when you enter it */
#define ENTRY_END 290 /* how far inside a room you land when you enter it */

#ifdef PWNCHUCK_EMU_CONTROLS
#define MAX_WALK_V_Q8 (6 << 8)
#define TILT_ACC_Q8 24    /* velocity gained per frame at full tilt */
#define FRICTION_SHIFT 4  /* coasting decay */
#define BRAKE_SHIFT 3     /* brake decay */
#else
#define MAX_WALK_V_Q8 (12 << 8)
#define TILT_ACC_Q8 48    /* velocity gained per frame at full tilt */
#define FRICTION_SHIFT 4  /* coasting decay */
#define BRAKE_SHIFT 2     /* brake decay */
#endif

// Tune gravity pull: how fast the walker falls
#define GRAVITY_Q8 3

void physics_reset(Player *p);
void physics_update(Player *p, const InputState *in);

#endif
