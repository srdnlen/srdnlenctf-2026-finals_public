#include "physics.h"
#include "rooms.h"

void physics_reset(Player *p){
	/* between the first two floor blocks in Pipe Hall: the walker must
	 * not start on top of one */
	p->px = 135;
	p->py = GROUND_Y - PLAYER_R; /* feet on the ground line */
	p->vx_q8 = 0;
	p->vy_q8 = 0;
	p->rem_x = 0;
	p->rem_y = 0;
	p->room = 0;
}

static s32 clamp32(s32 v, s32 lo, s32 hi){
	return v < lo ? lo : (v > hi ? hi : v);
}

/*
 * Solid props.  Each box pushes the walker out along whichever side it
 * entered by, and kills the velocity on that axis.  Positions are compared
 * in the same signed space the room edges use.
 */
static void collide_solids(Player *p, const RoomDef *def){
	u8 i;

	for (i = 0; i < def->solid_count && i < ROOM_SOLIDS_MAX; i++){
		const Solid *s = &def->solids[i];
		s16 px = (s16)p->px;
		s16 py = (s16)p->py;
		s16 left = (s16)s->x - PLAYER_R;
		s16 right = (s16)(s->x + s->w) + PLAYER_R;
		s16 top = (s16)s->y - PLAYER_R;
		s16 bottom = (s16)(s->y + s->h) + PLAYER_R;
		s16 dl, dr, dt, db;

		if (px <= left || px >= right || py <= top || py >= bottom)
			continue;

		dl = px - left;
		dr = right - px;
		dt = py - top;
		db = bottom - py;

		if ((dl < dr ? dl : dr) < (dt < db ? dt : db)){
			s16 x = (dl < dr) ? left : right;

			/*
			 * A push must never cross a wall: a walker who ends up
			 * inside a prop near the edge is sent back into the
			 * room, not through the wall.
			 */
			if (x < WALL_MARGIN + PLAYER_R &&
			    !(def->doors & DOOR_LEFT))
				x = right;
			if (x > ROOM_W - WALL_MARGIN - PLAYER_R &&
			    !(def->doors & DOOR_RIGHT))
				x = left;
			p->px = (u16)x;
			p->vx_q8 = 0;
			p->rem_x = 0;
		} else {
			s16 y = (dt < db) ? top : bottom;

			if (y < WALL_MARGIN + PLAYER_R)
				y = bottom;
			if (y > GROUND_Y - PLAYER_R)
				y = top;
			p->py = (u16)y;
			p->vy_q8 = 0;
			p->rem_y = 0;
		}
	}
}

void physics_update(Player *p, const InputState *in){
	const RoomDef *def = rooms_def(p->room);
	s32 step;

	/* Tilt is the only control: it pulls the walker around. */
	p->vx_q8 += ((s32)in->tilt_x * TILT_ACC_Q8) >> 8;
	p->vy_q8 -= ((s32)in->tilt_y * TILT_ACC_Q8) >> 8;

	if (in->tilt_x == 0)
		p->vx_q8 -= p->vx_q8 >> FRICTION_SHIFT;
	if (in->tilt_y == 0)
		p->vy_q8 -= p->vy_q8 >> FRICTION_SHIFT;

	if (in->brake){
		p->vx_q8 -= p->vx_q8 >> BRAKE_SHIFT;
		p->vy_q8 -= p->vy_q8 >> BRAKE_SHIFT;
	}


	p->vy_q8 += GRAVITY_Q8; // Adding the gravity coef 

	/* Cap the walking speed. */
	p->vx_q8 = clamp32(p->vx_q8, -MAX_WALK_V_Q8, MAX_WALK_V_Q8);
	p->vy_q8 = clamp32(p->vy_q8, -MAX_WALK_V_Q8, MAX_WALK_V_Q8);

	/*
	 * Integrate, carrying the sub-unit part: positions are whole units, so
	 * without the carry any speed below one unit per frame would round
	 * away to nothing.
	 */
	step = p->vx_q8 + p->rem_x;
	p->rem_x = (s16)(step % 256);
	p->px = (u16)(p->px + step / 256);

	step = p->vy_q8 + p->rem_y;
	p->rem_y = (s16)(step % 256);
	p->py = (u16)(p->py + step / 256);

	collide_solids(p, def);

	/* Walls: solid bands, except where the room opens into a doorway.
	 * Doorways are the only way out of a room. */
	if (!(def->doors & DOOR_LEFT) && p->px < WALL_MARGIN + PLAYER_R){
		p->px = WALL_MARGIN + PLAYER_R;
		p->vx_q8 = 0;
		p->rem_x = 0;
	}
	if (!(def->doors & DOOR_RIGHT) && p->px > ROOM_W - WALL_MARGIN - PLAYER_R){
		p->px = ROOM_W - WALL_MARGIN - PLAYER_R;
		p->vx_q8 = 0;
		p->rem_x = 0;
	}
	if (p->py < WALL_MARGIN + PLAYER_R){
		p->py = WALL_MARGIN + PLAYER_R;
		p->vy_q8 = 0;
		p->rem_y = 0;
	}
	if (p->py > GROUND_Y - PLAYER_R){
		p->py = GROUND_Y - PLAYER_R;
		p->vy_q8 = 0;
		p->rem_y = 0;
	}
}