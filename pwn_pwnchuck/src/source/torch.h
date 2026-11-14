/*
 * Torches and the light they cast.  Sixteen hang across the three rooms; the
 * walker takes the ones it rolls into and sets them down again one at a time.
 * Torches close to each other are what the light counts.
 */
#ifndef PWNCHUCK_TORCH_H
#define PWNCHUCK_TORCH_H

#include "types.h"
#include "game.h"

#define TORCH_COUNT 16

#define TORCH_LIGHT 16 // Quanta luce emette una torcia singola
#define TORCH_CLUSTER_RADIUS 24  // how close two torches must be to count as a cluster
#define TORCH_REACH 18      // how close the walker must be to take a torch

enum {
	TORCH_HOME = 0, // still on its bracket
	TORCH_HELD,     // carried by the walker
	TORCH_PLACED,   // set down
};

typedef struct {
	s16 room; // room the torch is standing in
	u16 x, y; // room local position, world units
	u8 state;
} Torch;

extern Torch g_torches[TORCH_COUNT];

void torches_init(void);               // every torch back on its bracket
void torches_reset_room(s16 room);     // one room's torches back home
bool torches_take_touched(Player *p);  // take the torch rolled into
bool torches_set_down(Player *p);      // set one carried torch down
u8 torches_held(void);                 // how many the walker carries

/* Light of the torches in a room: TORCH_LIGHT for every torch that has a
 * neighbour within TORCH_CLUSTER_RADIUS.  All sixteen together wrap it to 256. */
u16 torches_light(s16 room);

#endif
