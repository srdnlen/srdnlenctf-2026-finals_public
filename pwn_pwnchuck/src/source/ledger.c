#include <string.h>
#include "ledger.h"

/* The light: the intensity in the low byte, the debug flag in the high one. */
static u16 light; // Uno short che contiene sia l'intensità della luce (low byte) che il flag di debug (high byte). Da capire se si può convertire in struct

// Static, so a page starts out empty.
static u8 page[LEDGER_STEPS];
static u8 written;             // steps on this page so far
static u8 leaning;             // the way the walker was leaning last

void ledger_reset(void){
	light = 0;
	ledger_new_page();
}

void ledger_new_page(void){
	memset(page, 0, sizeof page);
	written = 0;
	leaning = 0xff; // nothing was leaned last
}

/* The low byte is the intensity and wraps at 256; the carry it hands the
* high byte is what raises the flag, with no flag to raise by hand. */
void ledger_set_light(u16 total){ light = total; } // Scrittura short (u16) su una struct di 2 u8.

// The only way to exploit the vuln is to have a u16 and have these two funcs for bitmasking :-(
u8 ledger_light(void){ return (u8)light; } // the low byte is the intensity
bool ledger_dbg_mode(void){ return (light >> 8) != 0; } // the high byte is the debug flag

static u8 direction_of(s16 tilt_x, s16 tilt_y){
	s16 ax = tilt_x < 0 ? (s16)-tilt_x : tilt_x;
	s16 ay = tilt_y < 0 ? (s16)-tilt_y : tilt_y;

	// the walker is not leaning at all
	if (ax == 0 && ay == 0) return 0xff;

	// a slight tilt is still a step
	if (ax >= ay)
		return tilt_x < 0 ? MOVE_LEFT : MOVE_RIGHT;
	return tilt_y > 0 ? MOVE_UP : MOVE_DOWN;
}

void ledger_note_tilt(s16 tilt_x, s16 tilt_y){
	u8 lean = direction_of(tilt_x, tilt_y);

	// if the walker is not leaning, or is leaning the same way as last time, do nothing
	if (lean == 0xff || lean == leaning) return;

	leaning = lean;
	if (written < LEDGER_STEPS) page[written++] = lean;
}

u8 ledger_step_count(void)      { return written;   }
const u8 *ledger_steps(void)    { return page;      }
