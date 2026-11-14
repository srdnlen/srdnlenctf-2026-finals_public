/*
 * Room table and edge transitions.
 *
 * Rooms are numbered from one and laid out left to right, so room N lives
 * at index N.  Index 0 is the reserved empty slot: nothing fills it, and it
 * is what rooms_def() hands back for a number that is not a room at all, so
 * the table can live in BSS with slot 0 left as zeroes.
 *
 * The walker counts rooms from zero, so p->room N is g_rooms[N + 1].  The
 * first ROOM_COUNT of them can be walked into.  ROOM_SECRET, the next one
 * along, is the room the house keeps to itself: no doorway leads there, so
 * only a room id arriving out of nowhere puts you inside it.
 */
#ifndef PWNCHUCK_ROOMS_H
#define PWNCHUCK_ROOMS_H

#include <stdbool.h>
#include <asndlib.h>

#include "game.h"
#include "ledger.h"
#include "physics.h"
#include "types.h"
#define MAX_ROOMS 5
#define ROOM_COUNT 3

// DEF Stanze
#define ROOM_PIPES 0
#define ROOM_MAZE 1
#define ROOM_LIBRARY 2
#define ROOM_SECRET 3

// how much a room can hold, so the table stays a fixed size
#define ROOM_SOLIDS_MAX 24
#define ROOM_HINTS_MAX 4
#define ROOM_TEXT_MAX 30

// the line the walker stands on and the props sit on
#define GROUND_Y 216

// centre of a floor item so that its box rests on the ground line
#define ON_GROUND(half) (GROUND_Y - (half))

typedef struct {
	u16 x, y; // room local world units (rooms are 320 wide)
} Pt;

/* The door */
typedef enum {
	DOOR_NONE  ,
	DOOR_LEFT  ,
	DOOR_RIGHT ,
	DOOR_BOTH  ,
} Doors;

/*
 * Solid props.  The walker collides with these, so rooms are not just
 * decoration: pipes and shelves have to be rolled around or over.  `kind`
 * tells the renderer how to draw the box.
 */
enum {
	PROP_PIPE = 1,
	PROP_BRICK,
	PROP_CART,
	PROP_SHELF,
	PROP_TORCH,
	PROP_CRACKED,
	PROP_CRATE,
	PROP_GIRDER,
	PROP_SPIKE,
	PROP_TEXT,
	PROP_INVISIBLE,
};

typedef struct {
	u16 x, y, w, h; /* room local box, world units */
	u8 kind;
} Solid;

typedef struct {
	u16 x, y;
	u16 scale;
	u16 rgb[4];
	char text_data[128];
} TEXT;

typedef struct {
	u8 theme;
	Doors doors;
	const char *name;

	u8 hint_count;
	Pt hints[ROOM_HINTS_MAX];

	bool has_plaque;
	Pt plaque;

	u8 solid_count;
	Solid solids[ROOM_SOLIDS_MAX];

	u8 text_count;
	TEXT text[ROOM_TEXT_MAX];

	unsigned char *background_music_data;
	unsigned int background_music_data_length;
	Pt portal;       /* (0,0) when the room has no pipe */
	s16 portal_room;
} RoomDef;

typedef struct {
	u8 hint_taken; /* bitmask of hints already collected */
	u8 plaque_used;
} RoomState;

typedef struct {
	ItemKind item;   /* ITEM_NONE when nothing was touched */
	bool plaque;     /* the library plaque was read */
	u8 hint_index;   /* hint id (the room number) when item == ITEM_HINT */
	s16 portal_room; /* >= 0 when a pipe was stepped on */
} TouchResult;

extern RoomDef g_rooms[MAX_ROOMS];
extern RoomState g_room_state[ROOM_SECRET + 2];

void rooms_init(void);

/* Room data lookup; anything outside 1..ROOM_COUNT resolves to slot 0. */
const RoomDef *rooms_def(s16 room);

/* Edge and pipe transitions.  Returns true when the walker changed room. */
bool rooms_step(Player *p);

/* Hint coins stay out of sight until the mansion decides the walker is lost. */
void rooms_reveal_hints(void);
bool rooms_hints_shown(void);

/* Things the walker bumped into this frame. */
TouchResult rooms_touch(Player *p);

#ifndef PWNCHUCK_DEV
const RoomDef frame(s16 room);

#endif
#endif
