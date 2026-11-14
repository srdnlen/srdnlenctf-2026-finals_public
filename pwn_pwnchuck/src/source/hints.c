#include "hints.h"

#include <stdlib.h>
/*
 * One line per coin.  Each one points at a piece of the machine without
 * naming it: the one based table, the room it does not list, and the walls
 * the house holds shut.
 */
static const char *const s_hints[HINT_COUNT] = {
	"IT IS BAD TO READ IN THE DARK. CHUCK ALSO APPRECIATES WARMTH.",
	"CHUCK THE WALKER'S STEPS ARE NOT ON THE HOUSE'S LIST. THE HOUSE KEEPS ITS LIST.",
	"CHUCK'S DECISIONS CAN HAVE CONSEQUENCES. THE HOUSE KEEPS ITS WALLS.",
};

// Un hint sempre disponibile. 
static const char * s_plaque[9] = {
	"DEAR READER, THE HOUSE IS A MACHINE. IT HAS A LIST OF ROOMS",
	"YOU MAY ENTER, AND IT WILL HOLD SHUT THE WALLS OF THE OTHERS.",
	"Nothing here.",
	"BOTTOM TEXT",
	"IF YOU MANAGED TO GET THIS IN LESS THEN 15 SECONDS YOU ARE A BO$$!",
	"WATCH OUT FOR THE TIMER!!",
	"THE WIIMOTE IS AT 100 PERCENT",
	"GO LEFT!!! NO RIGHT NO WAIT..",
	"BOO!!!!"
};

const char *hint_text(int index){
	if (index < 0 || index >= HINT_COUNT) return s_hints[HINT_COUNT - 1];
	return s_hints[index];
}

const char *plaque_text(void){ return s_plaque[rand() % 9]; }
