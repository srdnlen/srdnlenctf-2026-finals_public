#ifndef PWNCHUCK_LEDGER_H
#define PWNCHUCK_LEDGER_H

#include <stdbool.h>
#include "types.h"

/*
 * Steps written down in one room.  One of them sits past the end of the buffer next_room() keeps on its stack
 * Sarebbe il numero necessario per fare overflow del buffer di next_room() e sovrascrivere il real_room_id, che è un u8.
 * 32 passi sono sufficienti per scrivere un byte oltre la fine del buffer di 32 passi.
 */
#define LEDGER_STEPS 8


/* Gli ID delle direzioni in cui ci muoviamo */
#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define MOVE_UP 2
#define MOVE_DOWN 3

/*
 * The light is one 16-bit value: the low byte is the intensity the room
 * shows, the high byte is the "debug mode" flag.
*/
void ledger_reset(void);

/* The light the torches in the room cast, sixteen for every lit torch. */
void ledger_set_light(u16 total);
u8 ledger_light(void);

/* Set the moment the gathered light walks past the byte it is kept in. */
bool ledger_dbg_mode(void);

/*
 * A step is written down when the walker leans a way it was not leaning
 * before.  Leaning on the centre writes nothing down.
 */
void ledger_note_tilt(s16 tilt_x, s16 tilt_y);

/* The house starts a fresh page of steps for a room. */
void ledger_new_page(void);

u8 ledger_step_count(void);
const u8 *ledger_steps(void);

#endif
