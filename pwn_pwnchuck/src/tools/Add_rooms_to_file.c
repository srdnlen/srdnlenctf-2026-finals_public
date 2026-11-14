#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../source/audio/room_0_music.h"
#include "../source/audio/room_3_music.h"
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;


#define ROOM_COUNT 3
#define ROOM_SECRET 3
#define BLOCK_R 8
#define MAX_ROOM_NAME_LEN 32
// how much a room can hold, so the table stays a fixed size
#define ROOM_BLOCKS_MAX 16
#define MAX_ITEMS 16

#define ROOM_SOLIDS_MAX 24
#define ROOM_HINTS_MAX 4
#define ROOM_TEXT_MAX 30
// the line the walker stands on and the props sit on
#define GROUND_Y 216

// centre of a floor item so that its box rests on the ground line
#define ON_GROUND(half) (GROUND_Y - (half))

// how long a spent ? block stays dark before it can be popped again
#define BLOCK_RESPAWN_FRAMES 600


enum {
	THEME_DEBUG = 0, /* what the zeroed reserved slot draws with */
	THEME_SMB,
	THEME_DKC,
	THEME_LIB,
	THEME_ZELDA,
	THEME_COUNT,
};

typedef struct {
	u16 x, y; // room local world units (rooms are 320 wide)
} Pt;

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


enum {
	DOOR_LEFT = 1,
	DOOR_RIGHT = 2,
};

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
};


typedef struct {
	u16 x, y, w, h; /* room local box, world units */
	u8 kind;
} Solid;

/* What a room interaction produced.  A popped block always drops a coin. */

typedef struct {
	u16 x,y;
	u16 scale;
	u16 rgb[4];
	char text_data [128];
	
} TEXT;

typedef struct Pickable {
	Pt pos;
    //ItemKind kind;
	//Sprite *sprite;
	const u8 (*palette)[3];
	
	float scale;
	u8 range;
	
} Pickable;

typedef struct {
	u8 theme;
	u8 doors;
	char name[MAX_ROOM_NAME_LEN];

	u8 block_count;
	Pt blocks[ROOM_BLOCKS_MAX];

	u8 item_count;
	Pickable items[MAX_ITEMS];

	u8 hint_count;
	Pt hints[ROOM_HINTS_MAX];

	bool has_plaque;
	Pt plaque;

	u8 solid_count;
	Solid solids[ROOM_SOLIDS_MAX];
	
	u8 text_count;
	TEXT text[ROOM_TEXT_MAX];
	
	unsigned char * background_music_data;
	unsigned int background_music_data_length;
	Pt portal;       /* (0,0) when the room has no pipe */
	s16 portal_room;
} RoomDef;

typedef struct {
	u32 block_used[ROOM_BLOCKS_MAX]; /* frame the block popped plus one, 0 = ready */
	u8 hint_taken;                   /* bitmask of hints already collected */
	u8 plaque_used;
} RoomState;

typedef struct {
	//ItemKind item;   /* ITEM_NONE when nothing was touched */
	bool plaque;     /* the library plaque was read */
	u8 hint_index;   /* hint id (the room number) when item == ITEM_HINT */
	s16 portal_room; /* >= 0 when a pipe was stepped on */
} TouchResult;

void write_room(RoomDef room, FILE *f,int i) {
		printf("[%d] Writing theme %d:\n", i, room.theme);
		fwrite(&(room.theme), sizeof(room.theme), 1, f);
		
		printf("[%d] Writing doorcount %d:\n", i, room.doors);
		fwrite(&(room.doors), sizeof(room.doors), 1, f);
		
		u32 room_name_len = strlen(room.name);
		printf("[%d] Writing name length %d:\n", i, room_name_len);
		fwrite(&(room_name_len), sizeof(u32), 1, f);
		
		printf("[%d] Writing name %s:\n", i, room.name);
		fwrite(&(room.name), sizeof(u8), room_name_len, f);
		
		printf("[%d] Writing block count %d:\n", i, room.block_count);
		fwrite(&(room.block_count), sizeof(room.block_count), 1, f);
	
		printf("[%d] Writing block position\n", i);
		fwrite(&(room.blocks), sizeof(Pt), room.block_count, f);

		printf("[%d] Writing hint count %d:\n", i, room.hint_count);
		fwrite(&(room.hint_count), sizeof(room.hint_count), 1, f);

		printf("[%d] Writing hint position\n", i);
		fwrite(&(room.hints), sizeof(Pt), room.hint_count, f);

		printf("[%d] Writing plaque %d:\n", i, room.has_plaque);
		fwrite(&(room.has_plaque), sizeof(bool), 1, f);
		
		printf("[%d] Writing plaque position\n", i);
		fwrite(&(room.plaque), sizeof(Pt), 1, f);

		printf("[%d] Writing solid count %d:\n", i, room.solid_count);
		fwrite(&(room.solid_count), sizeof(room.solid_count), 1, f);

		printf("[%d] Writing solid position\n", i);
		fwrite(&(room.solids), sizeof(Solid), room.solid_count, f);

		printf("[%d] Writing text count %d:\n", i, room.text_count);
		fwrite(&(room.text_count), sizeof(room.text_count), 1, f);

		printf("[%d] Writing text data\n", i);
		for(int j = 0; j<room.text_count; j++){
			printf("[%d] Writing text %d: %s\n", i, j, room.text[j].text_data);
			fwrite(&(room.text[j]), sizeof(TEXT), 1, f);
		}
		
		printf("[%d] Writing Music index: \n",i);
		fwrite(&(room.background_music_data), sizeof(room.background_music_data), 1, f);

		printf("[%d] Writing music length: %d\n",i,room.background_music_data_length);
		fwrite(&(room.background_music_data_length), sizeof(room.background_music_data_length), 1, f);
		
		printf("[%d] Writing portal position:\n",i);
		fwrite(&(room.portal), sizeof(room.portal), 1, f);
		
		printf("[%d] Writing portal portal_room:\n\n",i);
		fwrite(&(room.portal_room), sizeof(room.portal_room), 1, f);	
		
	}

void read_room(RoomDef *room, FILE *f,int i) {
		fread(&(room->theme), sizeof(room->theme), 1, f);
		printf("[%d]Theme Value:  Value: %d:\n", i, room->theme);
		
		fread(&(room->doors), sizeof(room->doors), 1, f);
		printf("[%d]doorcount  Value: %d:\n", i, room->doors);
		
		u32 string_len;
		fread(&string_len, sizeof(string_len),1, f);
		fread(&(room->name), sizeof(u8), string_len, f);

		printf("[%d]name  Value: %s:\n", i, room->name);
		
		fread(&(room->block_count), sizeof(room->block_count), 1, f);
		printf("[%d]block count  Value: %d:\n", i, room->block_count);

		fread(&(room->blocks), sizeof(Pt), room->block_count, f);
		printf("[%d]block position\n", i);
		
		for(int k = 0; k<room->block_count;k++){
			printf("{%d,%d}, ",room->blocks[k].x,room->blocks[k].y);
		}
		
		fread(&(room->hint_count), sizeof(room->hint_count), 1, f);
		printf("\n[%d]hint count  Value: %d:\n", i, room->hint_count);

		fread(&(room->hints), sizeof(Pt), room->hint_count, f);
		printf("[%d]hint position\n", i);

		fread(&(room->has_plaque), sizeof(u8), 1, f);
		printf("[%d]plaque  Value: %d:\n", i, room->has_plaque);
		
		fread(&(room->plaque), sizeof(Pt), 1, f);
		printf("[%d]plaque position\n", i);

		fread(&(room->solid_count), sizeof(room->solid_count), 1, f);
		printf("[%d]solid count  Value: %d:\n", i, room->solid_count);

		fread(&(room->solids), sizeof(Solid), room->solid_count, f);
		printf("[%d]solid position\n", i);

		fread(&(room->text_count), sizeof(room->text_count), 1, f);
		printf("[%d]text count  Value: %d:\n", i, room->text_count);

		printf("[%d]text data\n", i);
		for(int j = 0; j<room->text_count; j++){
			fread(&(room->text[j]), sizeof(TEXT), 1, f);
			printf("[%d]text  Value: %d:  Value: %s\n", i, j, room->text[j].text_data);

		}
		
		fread(&(room->background_music_data), sizeof(room->background_music_data), 1, f);
		printf("[%d] Writing Music index: \n",i);

		fread(&(room->background_music_data_length), sizeof(room->background_music_data_length), 1, f);
		printf("[%d] Writing music length:  Value: %d\n",i,room->background_music_data_length);

		fread(&(room->portal), sizeof(room->portal), 1, f);
		printf("[%d] Writing portal position:\n",i);

		fread(&(room->portal_room), sizeof(room->portal_room), 1, f);	
		printf("[%d] Writing portal portal_room:\n\n",i);
		
	}
int main(){

    RoomDef rooms[ROOM_SECRET + 2];
    RoomDef rooms1[ROOM_SECRET + 2];

    rooms[0] = (RoomDef){

    };
	/* Room 0: Pipe Hall. */
	rooms[1] = (RoomDef){
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
		.block_count = 6,
		.blocks = {{60, ON_GROUND(BLOCK_R)}, {110, ON_GROUND(BLOCK_R)},
			   {160, ON_GROUND(BLOCK_R)}, {210, ON_GROUND(BLOCK_R)},
			   {100, 100}, {250, 56}},
		.hint_count = 1,
		.hints = {{30, 96}},
		.portal_room = -1,
		//.background_music_data = room_0_music_data,
		//.background_music_data_length = room_0_music_data_len,
	};

	/* Room 1: Kong's Maze.  */
	rooms[2] = (RoomDef){
		.theme = THEME_DKC,
		.doors = DOOR_LEFT | DOOR_RIGHT,
		.name = "KONG'S MAZE",
		.solid_count = 18,
		.solids = {
			/* posts: one per blocked column boundary, filling the
			 * gap between the beams above and below it */
			{175, 8, 10, 36, PROP_GIRDER},
			{89, 54, 10, 31, PROP_GIRDER},
			{261, 54, 10, 31, PROP_GIRDER},
			{46, 95, 10, 31, PROP_GIRDER},
			{132, 95, 10, 31, PROP_GIRDER},
			{218, 95, 10, 31, PROP_GIRDER},
			{132, 136, 10, 31, PROP_GIRDER},
			{261, 136, 10, 31, PROP_GIRDER},
			{175, 177, 10, 36, PROP_GIRDER},
			/* beams: one per run of blocked row boundaries */
			{51, 44, 86, 10, PROP_GIRDER},
			{223, 44, 43, 10, PROP_GIRDER},
			{8, 85, 43, 10, PROP_GIRDER},
			{137, 85, 43, 10, PROP_GIRDER},
			{223, 85, 43, 10, PROP_GIRDER},
			{51, 126, 43, 10, PROP_GIRDER},
			{137, 126, 86, 10, PROP_GIRDER},
			{8, 167, 129, 10, PROP_GIRDER},
			{180, 167, 86, 10, PROP_GIRDER},
		},
		.block_count = 13,
		.blocks = {{72, 29}, {29, 29}, {158, 29}, {158, 70}, {72, 111},
			   {244, 111}, {158, 152}, {201, 152}, {244, 152},
			   {287, 111}, {287, 29}, {72, 193}, {115, 193}},
		.hint_count = 1,
		.hints = {{201, 193}},
		.portal_room = -1,
	};

	/* Room 2: Layton's Library. */
	rooms[3] = (RoomDef){
		.theme = THEME_LIB,
		.doors = DOOR_LEFT | DOOR_RIGHT,
		.name = "LAYTON'S LIBRARY",
		.solid_count = 3,
		.solids = {
			{40, 60, 50, 120, PROP_SHELF},
			{130, 60, 50, 120, PROP_SHELF},
			{220, 60, 50, 120, PROP_SHELF},
		},
		//.item_count = 1,
		//.items = {
		//{{130,60},ITEM_TORCH, &art_torch,pal_torch, 0.5, 1},
		//},

		.block_count = 3,
		.blocks = {{110, 150}, {200, 150}, {300, 100}},
		.hint_count = 1,
		.hints = {{40, 32}},
		.has_plaque = true,
		.plaque = {280, ON_GROUND(16)},
		.portal_room = -1,
		.background_music_data = room_3_music_data,
		.background_music_data_length = room_3_music_data_len,
	};

	/* Room 3, WIN_ROOM: Torch Temple. */
	rooms[4] = (RoomDef){
		.theme = THEME_ZELDA,
		.doors = 0,
		.name = "TROPHY ROOM",
		.solid_count = 3,
		.solids = {
		},
		.text_count = 3,
		.text= {
				{320, 120, 2,{69, 74, 70},"Hey Ale maybe we should remove this room.."},
				{120, 140, 2, {69, 74, 70}, "it's useless"} ,
				{320, 160, 1,{69, 74, 70},"You know... maybe someone can access it and steal our secret source"},

		},
		/* nothing to collect in here */
		.block_count = 0,
		.portal = {0, 0},
		.portal_room = -1,
	};


	printf("Writing data to file...\n");
    FILE *f = fopen("./rooms.bin", "wb");

	u32 room_count = ROOM_SECRET + 1;
	printf("Total Rooms: %d \n",room_count);

	fwrite(&room_count, sizeof(room_count), 1, f);
	for (int i = 0; i <= room_count; i++){
		printf("\nWriting room %d:\n", i);
		write_room(rooms[i], f,i);

	}
	fclose(f);



    printf("\nRooms written to rooms.bin\nChecking the file...\n");
  
	FILE *f1 = fopen("./rooms.bin", "rb");

	u32 rc = 0;
	fwrite(&rc, sizeof(rc), 1, f);

	printf("Total Rooms: %d \n",rc);

	fread(&rc, sizeof(room_count), 1, f);
	for (int i = 0; i <= rc; i++){
		printf("\nReading room %d:\n", i);
		read_room(&rooms1[i], f1,i);	
	}    
	
	return 0;
}