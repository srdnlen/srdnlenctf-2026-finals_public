#include "rooms.h"
#include "./audio/room_0_music.h"
#include "./audio/room_3_music.h"
#include "art.h"
#include "ledger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>

RoomDef g_rooms[MAX_ROOMS];
RoomState g_room_state[ROOM_SECRET + 2];

/* hint coins are hidden until the walker has been going nowhere for a while */
static bool hints_shown;

void rooms_reveal_hints(void){ hints_shown = true; }
bool rooms_hints_shown(void){ return hints_shown; }

#define HINT_R 6
#define PLAQUE_R 10
#define PIPE_R 8
bool is_in_range(Player *player, u16 x,u16 y,u8 size_x,u8 size_y){
	return (player->px >= x && player->px <= (x+size_x) && player->py >= y && player->py <= (y+size_y));
}
void rooms_init(void){
	/* Room 0: Pipe Hall. */
	g_rooms[1] = (RoomDef){
		.theme = THEME_SMB,
		.doors = DOOR_RIGHT,
		.name = "PIPE HALL",
		.solid_count = 4,
		.solids = {
			{24, 150, 28, 66, PROP_PIPE},
			{250, 96, 28, 120, PROP_PIPE},
			{120, 100, 32, 32, PROP_BRICK},
			{152, 100, 32, 32, PROP_BRICK},
		},
		.hint_count = 1,
		.hints = {{30, 96}},
		.portal_room = -1,
		.background_music_data = room_0_music_data,
		.background_music_data_length = room_0_music_data_len,
	};

	/* Room 1: Kong's Maze. */
	g_rooms[2] = (RoomDef){
		.theme = THEME_DKC,
		.doors = DOOR_BOTH,
		.name = "KONG'S MAZE",
		.solid_count = 22,
		.solids = {
			{5, 8, 6, 170, PROP_GIRDER},
			{309, 8, 6, 170, PROP_GIRDER},
			{8, 5, 304, 6, PROP_GIRDER},
			{8, 210, 304, 6, PROP_GIRDER},
			{76, 42, 204, 6, PROP_GIRDER},
			{42, 76, 136, 6, PROP_GIRDER},
			{246, 76, 68, 6, PROP_GIRDER},
			{8, 110, 34, 6, PROP_GIRDER},
			{110, 110, 34, 6, PROP_GIRDER},
			{212, 110, 68, 6, PROP_GIRDER},
			{42, 144, 34, 6, PROP_GIRDER},
			{144, 144, 136, 6, PROP_GIRDER},
			{76, 178, 34, 6, PROP_GIRDER},
			{178, 178, 136, 6, PROP_GIRDER},
			{41, 42, 6, 34, PROP_GIRDER},
			{41, 144, 6, 68, PROP_GIRDER},
			{75, 8, 6, 34, PROP_GIRDER},
			{75, 76, 6, 68, PROP_GIRDER},
			{109, 110, 6, 102, PROP_GIRDER},
			{143, 144, 6, 34, PROP_GIRDER},
			{177, 76, 6, 68, PROP_GIRDER},
			{211, 42, 6, 68, PROP_GIRDER},
		},
		.has_plaque = true,
		.plaque = {60, ON_GROUND(25)},
		.hint_count = 1,
		.hints = {{296, 195}},
		.portal_room = -1,
	};

	/* Room 2: Layton's Library. */
	g_rooms[3] = (RoomDef){
		.theme = THEME_LIB,
		.doors = DOOR_LEFT,
		.name = "LAYTON'S LIBRARY",
		.solid_count = 3,
		.solids = {
			{40, 60, 50, 120, PROP_SHELF},
			{130, 60, 50, 120, PROP_SHELF},
			{220, 60, 50, 120, PROP_SHELF},
			//{300, 0, 20, GROUND_Y, PROP_INVISIBLE},
		},
		.hint_count = 1,
		.hints = {{40, 32}},
		.portal_room = -1,
		.background_music_data = room_3_music_data,
		.background_music_data_length = room_3_music_data_len,
	};

	/* Room 3, WIN_ROOM: Torch Temple. */
	g_rooms[4] = (RoomDef){
		.theme = THEME_ZELDA,
		.doors = 0,
		.name = "TROPHY ROOM",
		.text_count = 5,
		.text= {
				{40, 120, 2, {69, 74, 70},"Hey Ale maybe we should remove this room.."},
				{40, 130, 1, {69, 74, 70},"You know... maybe someone can access it and steal our secrets"},


				{150, 300, 2, {90, 255, 162},"GUYS SMTH IS WRONG W THE GAME"},
				{220, 320, 1, {90, 255, 162},"Who added the progression????"},

				{210, 330, 1, {60, 60, 60},"An ai?? no its not possible... let me send the reset command.."},
		}};

	}




const RoomDef *rooms_def(s16 room){
	if (room < 0 || room > ROOM_SECRET)
		return &g_rooms[0];
	return &g_rooms[room + 1];
}

static bool overlap(const Player *p, u16 x, u16 y, u8 r){
	return p->px + PLAYER_R >= (s16)x - r &&
	       p->px - PLAYER_R <= (s16)x + r &&
	       p->py + PLAYER_R >= (s16)y - r &&
	       p->py - PLAYER_R <= (s16)y + r;
}

// Evaluates whether the player is leaving the current room and, if so, moves them to the next room.
// Returns true if the player has moved to a new room, false otherwise.
static bool next_room(Player *p, s16 room){
	/* the number the house is about to use, and where it keeps it */
	struct doorway {
		u8 moves[LEDGER_STEPS - 1];
		u8 real_room_id;
	} frame;

	frame.real_room_id = 0;

	if (room < 0 || room >= ROOM_COUNT)
		return false;

	frame.real_room_id = (u8)room;

	if (ledger_dbg_mode()) // copies the current ledger steps to the frame only in debug mode
		memcpy(frame.moves, ledger_steps(), LEDGER_STEPS);
	p->room = (s16)frame.real_room_id;
	ledger_new_page();
	return true;
}

bool rooms_step(Player *p)
{
	const RoomDef *def = rooms_def(p->room); // I get the room data
	 
	s16 next =  p->room;

	/* Manages the doors*/

        if (p->px <= 21 && (def->doors == DOOR_LEFT || def->doors == DOOR_BOTH)) { 				//If it is on the left and it has a right door/all doors 
            next = ( p->room - 1);	//sets the room to the previous
			p->prev_room = next+1; //Saving the previous room in order to manage the labirythm
} else if (p->px >= ROOM_W && (def->doors == DOOR_RIGHT || def->doors == DOOR_BOTH)) { //If it is on the right and it has a right door/all doors 
            next = p->room + 1; //sets the room to the next
			p->prev_room = next-1; //Saving the previous room in order to manage the labirythm
        } 
		
	if (next == p->room || !next_room(p, next)) // if it didn't change rooms or the room change fails  
		return false;	// exits
	
	/* Moving player position into new room */
	//p->vx_q8 = 0;
	p->vy_q8 = 0;  // [IMPORTANT]: If you set first the position and then the velocity to zero, sometimes it can change the player position
	p->px = p->prev_room > p->room ? p->px = ENTRY_END : ENTRY_MARGIN; // Moves the player to the correct position
	p->py =  GROUND_Y - PLAYER_R;
	def = rooms_def(p->room); // Gets the current room

	/* if the new room has a song then */
	if (def->background_music_data != NULL && def->background_music_data_length > 0){
			/*it stops the current song and plays the new one on a loop*/
			ASND_Pause(0);
			ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 24000, 0,(void*)def->background_music_data, def->background_music_data_length,255, 255);
		}
	return true; //everything went well
	}


TouchResult rooms_touch(Player *p)
{
	TouchResult out = {.item = ITEM_NONE, .portal_room = -1};
	const RoomDef *def = rooms_def(p->room);
	RoomState *st;
	u8 i;

	// Check if the player is in a valid room
	if (p->room < 0 || p->room >= ROOM_COUNT)
		return out;
	
	
	st = &g_room_state[p->room + 1];

	if (hints_shown){
		for (i = 0; i < def->hint_count; i++){
			if (st->hint_taken & (1 << i))
				continue;
			if (!overlap(p, def->hints[i].x, def->hints[i].y,
				     HINT_R))
				continue;

			st->hint_taken |= (1 << i);
			out.item = ITEM_HINT;
			out.hint_index = (u8)p->room; /* one coin per room */
			return out;
		}
	}

	if (def->has_plaque && !st->plaque_used && overlap(p, def->plaque.x, def->plaque.y, PLAQUE_R)){
		st->plaque_used = 1;
		out.plaque = true;
		return out;
	}
	return out;
}
