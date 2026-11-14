/*
 * PwnChuck: world update, rendering and the title screen.
 *
 * The room fills the screen at 2x.  The ground band along the bottom is the
 * status panel: room name and counters on the first row, the current message
 * on the rows below.  Nothing the walker can reach is drawn behind it, so
 * the play area stays clear.
 */
#include <stdio.h>
#include <string.h>

#include <wiiuse/wpad.h>

#include "./audio/flame.h"
#include "./audio/Maze_low_time.h"
#include "./audio/Maze_time.h"
#include "./audio/secret_room_audio.h"

#include "art.h"
#include "flag.h"
#include "game.h"
#include "gx2d.h"
#include "hints.h"
#include "input.h"
#include "ledger.h"
#include "physics.h"
#include "rooms.h"
#include "sprite.h"
#include "text.h"
#include "torch.h"
#include <unistd.h>

/* -layout */
#define MAX_ROOMS 5
#define SCALE 2 /* world units to screen pixels */
#define PX(v) ((v) * SCALE)
#define SCREEN_W (ROOM_W * SCALE)
#define SCREEN_H (ROOM_H * SCALE)
// FORSE SU WII VA A 50 FPS, QUINDI IL TIMER DI 30 SECONDI SAREBBE 30 * 50 = 1500 FRAME. DA VERIFICARE.
#ifdef PWNCHUCK_EMU_CONTROLS
#define MAZE_TIMER (30 * 60) // Timer di 30 secondi per Kong's Maze, in frame (60 fps). Da verificare se su Wii gira a 60 o 50 fps. (NTSC vs PAL)
#define MAZE_LOW_TIME ((30-15+1)*60)    // Testo rosso e musica di allarme quando mancano 15 secondi, in frame (60 fps)
#else
#define MAZE_TIMER (45 * 60)
#define MAZE_LOW_TIME ((45-30+1)* 60)
#endif

/* the number of frames needed to make the game play the hard song */

#define PANEL_Y PX(GROUND_Y) /* the status panel starts at the ground line */
#define PANEL_GRASS 6        /* ground left showing along its top edge */
#define PANEL_X 16
#define ROW1_Y (PANEL_Y + 10) /* room name and counters: 2x text */
#define ROW2_Y (PANEL_Y + 30) /* message: 1x text, wraps to a second line */
#define MSG_WRAP 76           /* characters that fit on one 1x line */

/* state */

#define BURST_MAX 6
#define BURST_LIFE 44
/* A minute without progress and the hint coins surface. */
#define HINT_IDLE_FRAMES (60 * 60)

typedef struct {
	u8 kind;  /* ItemKind, 0 when the slot is free */
	u16 x, y; /* world units */
	u8 life;
} Burst;
static Player player;
static u32 frames;
bool secret_room_playing = false;
static u32 idle_frames; /* frames since the last sign of progress */
static u8 hints_taken;
static bool prev_action; /* the action button, last frame */

static const char *message;
static u32 message_frames;
static Burst bursts[BURST_MAX];

static bool show_title;

#ifdef PWNCHUCK_DEV
static bool debug;
static u32 seen_pressed; /* every button seen, shown by the overlay */
static u32 prev_btns;
#endif

/* helpers */

static void quad(int x, int y, int w, int h, u8 r, u8 g, u8 b){
	if (w <= 0 || h <= 0)
		return;
	gx2d_quad(x, y, w, h, r, g, b, 0xff);
}

static void say(const char *text, u32 ticks){
	message = text;
	message_frames = ticks;
}

static void spawn_burst(u8 kind, u16 x, u16 y){
	u8 i;

	for (i = 0; i < BURST_MAX; i++){
		if (bursts[i].kind)
			continue;
		bursts[i].kind = kind;
		bursts[i].x = x;
		bursts[i].y = y;
		bursts[i].life = BURST_LIFE;
		return;
	}
}

static void update_bursts(void){
	u8 i;

	for (i = 0; i < BURST_MAX; i++){
		if (!bursts[i].kind)
			continue;
		if (--bursts[i].life == 0)
			bursts[i].kind = 0;
	}
}

/* the world */

void game_init(void){
	ASND_Pause(0);
	ASND_Pause(1);

	rooms_init();
	torches_init();
	physics_reset(&player);
	idle_frames = 0;
	hints_taken = 0;
	ledger_reset();

	player.remaining_frames = MAZE_TIMER;

	memset(bursts, 0, sizeof bursts);
	show_title = true;
}

static void game_respawn(void){

	physics_reset(&player);
	ledger_new_page();
	torches_init();
	player.room = 0;
	idle_frames = 0;
	say("BACK TO THE PIPE HALL", 150);
	const RoomDef *pipehall = rooms_def(0);
	ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 24000, 0,
		(void*)pipehall->background_music_data, pipehall->background_music_data_length,
		255, 255);
}

// Rispawna Chuck all'inizio del labirinto e rimetti a posto tutte le torce che aveva raccolto.
static void maze_restart(void){
	torches_reset_room(ROOM_MAZE);
	if(player.prev_room == 0) player.px = ENTRY_MARGIN; // Rispawna Chuck all'inizio del labirinto se prima stava all'inizio
	else player.px = ENTRY_END; // Rispawna Chuck alla fine del labirinto

	player.vx_q8 = 0; // Resetta la velocità orizzontale e verticale
	player.vy_q8 = 0;
	player.remaining_frames = MAZE_TIMER; // Resetta il timer
	ledger_new_page(); // Resetta il ledger
	say("WHERE THE CHUCK WERE YOU PLANNING TO GO?", 180);
}

static void draw_timer(void){
	char buf[6];
	u8 r = 255, g = 255, b = 255; // Testo bianco di default

	// Se il timer è sotto i 15 secondi, cambia il colore del testo a rosso
	if (player.remaining_frames <= MAZE_LOW_TIME){
	    r = 255; // Invariato
		g = 0;
		b = 0;
	}
	snprintf(buf, sizeof(buf), "%d", (u32)(player.remaining_frames / 60));
	text_draw(SCREEN_H + 60, 35, 3, r, g, b, buf, MAIN_FONT);
}

#ifdef PWNCHUCK_DEV
void game_toggle_debug(void){
	debug = !debug;
}

bool game_debug_enabled(void){
	return debug;
}

/* Development cheats, only compiled into `make dev` builds. */
static void dev_cheats(const InputState *in){
	u32 pressed = in->btns_raw & ~prev_btns;

	prev_btns = in->btns_raw;

	if (pressed & WPAD_BUTTON_PLUS){ /* +: pile every torch here */
		u8 i;

		for (i = 0; i < TORCH_COUNT; i++){
			g_torches[i].room = player.room;
			g_torches[i].x = (u16)((s16)player.px + (i % 4) * 4);
			g_torches[i].y = (u16)((s16)player.py + (i / 4) * 4);
			g_torches[i].state = TORCH_PLACED;
		}
		say("DEV: TORCHES PILED", 120);
	}
	if (pressed & WPAD_BUTTON_MINUS){ /* -: skip to the next room */
		player.room = (s16)((player.room + 1) % ROOM_COUNT);
		player.px = ROOM_W / 2;
		player.py = GROUND_Y - PLAYER_R;
		player.vx_q8 = 0;
		player.vy_q8 = 0;
		ledger_new_page();
		say("DEV: NEXT ROOM", 120);
	}
	if (pressed & WPAD_BUTTON_A){ /* home: straight to the flag room */
		player.room = ROOM_SECRET;
		player.px = ROOM_W / 2;
		player.py = GROUND_Y - PLAYER_R;
		player.vx_q8 = 0;
		player.vy_q8 = 0;
		say("DEV: SECRET ROOM", 120);
	}
}
#endif

void game_update(const InputState *in){
	TouchResult touch;
	bool action = in->action && !prev_action;

	prev_action = in->action;
	frames++;

	if (show_title){
		/* any button starts the game */
		if (in->btns_pressed & (WPAD_BUTTON_A | WPAD_BUTTON_B |
					WPAD_BUTTON_1    | WPAD_BUTTON_2 |
					WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS)){
			show_title = false;
			say("FIND THE ROOM THAT EVERYONE FORGOT", 600);
			rooms_step(&player);
				const RoomDef *def = rooms_def(player.room);
				if(def->background_music_data != NULL && def->background_music_data_length >= 0){
					ASND_Pause(0);
						ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 24000, 0,
							(void*)def->background_music_data, def->background_music_data_length,
							255, 255);
			}
		}
		return;
	}

	if (in->reset_requested)
		game_respawn();

	#ifdef PWNCHUCK_DEV
		seen_pressed |= in->btns_pressed;
		dev_cheats(in);
	#endif

	/* The walker takes torches by rolling into them; the action button sets
	 * the armful down again, close together. */
	if (action && torches_set_down(&player)){
		say("TORCHES SET DOWN", 120);
		set_rumble(0,30,RUMBLE_STRONG);
		idle_frames = 0;
	}

	physics_update(&player, in);

	if (torches_take_touched(&player)){
		set_rumble(0,20,RUMBLE_MEDIUM);
		say("TORCH TAKEN", 120);
		spawn_burst(ITEM_TORCH, player.px, player.py);
		ASND_SetVoice(1, VOICE_STEREO_16BIT, 48000, 0,
			      (void*)flame_sound_data, flame_sound_data_length,
			      255, 255, NULL);
		idle_frames = 0;
	}

	/* Read the light before the walker can leave: the doorway uses it. */
	ledger_set_light(torches_light(player.room));

	/* the house writes the step down as it happens */
	ledger_note_tilt(in->tilt_x, in->tilt_y);

	touch = rooms_touch(&player);

	if (touch.item != ITEM_NONE || touch.plaque) idle_frames = 0; /* something happened: not stuck any more */

	if (touch.item == ITEM_HINT){
		set_rumble(0,20,RUMBLE_LOW);
		hints_taken |= (u8)(1u << touch.hint_index);
		say(hint_text(touch.hint_index), 900);
		spawn_burst(ITEM_HINT, player.px, player.py);
	}

	if (touch.plaque){
		say(plaque_text(), 900);
		set_rumble(0,10,RUMBLE_MEDIUM);

	}

	 if(rooms_step(&player)){
	 	idle_frames = 0; /* a new room counts as progress */
	 	if (player.room == ROOM_MAZE){
	 			
	 			player.py = GROUND_Y - PLAYER_R;
	 			player.remaining_frames = MAZE_TIMER;
	 		}
	 }

	// Metti fretta a chuck quando è nel labirinto
	if (player.room == ROOM_MAZE){
		if (player.remaining_frames > 0) {
			// Metti fretta a chuck quando è nel labirinto con la musica
			if(player.remaining_frames >= MAZE_TIMER){

				ASND_Pause(0); //stoppo la musica 
				ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 24000, 0,(void*)&Maze_time_raw, Maze_time_raw_len,200, 200); //metto in play la musica normale 
			}
			// Fai scangare la pantalogna quando chuck quando è nel labirinto
			if(player.remaining_frames <= MAZE_LOW_TIME){
				/* Se il tempo è poco allora fai vibrare e cambia canzone */
				if(player.remaining_frames == MAZE_LOW_TIME){
					set_rumble(0,60,RUMBLE_STRONG);
					ASND_Pause(0); //stoppo la musica 
					ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 24000, 0,(void*)&maze_no_time_raw, maze_no_time_raw_len,200, 200); //metto in play la musica velocizzata 
				}else{

					/*Se il tempo è tra i 10 secondi e il critico fai vibrare ogni secondo il controller */
					if(player.remaining_frames > 600 && player.remaining_frames % 60 == 0 )	 								          set_rumble(0,50,RUMBLE_STRONG);
					else if ((player.remaining_frames <= 600 && player.remaining_frames > 300) && player.remaining_frames % 50 == 0 ) set_rumble(0,40,RUMBLE_STRONG);
					else if ((player.remaining_frames <= 300 && player.remaining_frames > 60)  && player.remaining_frames % 30 == 0 ) set_rumble(0,20,RUMBLE_STRONG);
					else if (player.remaining_frames == 60) set_rumble(0,60,RUMBLE_STRONG);


				}
			}
 
		}
		if (player.remaining_frames > 0) player.remaining_frames--;
		if (player.remaining_frames == 0) maze_restart();

	}

	/* the light of the room the walker is now standing in */
	ledger_set_light(torches_light(player.room));
	if (!rooms_hints_shown() && ++idle_frames >= HINT_IDLE_FRAMES){
		rooms_reveal_hints();
		say("THREE COINS SHINE IN THE DUST", 300);
		spawn_burst(ITEM_HINT, player.px, player.py);
		idle_frames = 0;
	}else idle_frames++;

	/* the flag text only exists while the secret room is on screen */
		if (player.room == ROOM_SECRET){
		//controllo se sono già entrato in caso non suono più la musica
		if(secret_room_playing == false){
			secret_room_playing = true;
			ASND_Pause(0); //stoppo la musica 
			ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 24000, 0,(void*)&secret_raw, secret_raw_len,200, 200); //metto in play la musica normale 
		}
	}
	else
	{
		secret_room_playing = false;
		flag_wipe();
	}
	if (message_frames)
		message_frames--;
	update_bursts();
}

/* -------------------------------------------------------------- room paint */

static void draw_background(u8 theme){
	const Theme *t = &g_themes[theme];

	quad(0, 0, SCREEN_W, SCREEN_H, t->sky[0], t->sky[1], t->sky[2]);
}

/*
 * Doorways read as openings, not walls: a dark passage, a lit edge and an
 * arrow pointing out.  The floor is painted over their lower half so the
 * ground runs through the opening.
 */
static void draw_openings(const RoomDef *def){
	if (def->doors & DOOR_LEFT){
		quad(0, 0, PX(14), SCREEN_H, 14, 12, 20);
		quad(PX(14) - 3, 0, 3, SCREEN_H, 214, 202, 150);
		sprite_draw(&art_arrow_left, 2, PX(GROUND_Y) - 44, 3, pal_arrow);
	}
	if (def->doors & DOOR_RIGHT){
		quad(SCREEN_W - PX(14), 0, PX(14), SCREEN_H, 14, 12, 20);
		quad(SCREEN_W - PX(14), 0, 3, SCREEN_H, 214, 202, 150);
		sprite_draw(&art_arrow_right, SCREEN_W - PX(14) + 4,
			    PX(GROUND_Y) - 44, 3, pal_arrow);
	}
}

/*
 * The floor and the status panel share the bottom band: the ground line sits
 * at PX(GROUND_Y) and the panel covers everything below it.
 */
static void draw_ground(u8 theme){
	const Theme *t = &g_themes[theme];
	int y = PX(GROUND_Y);

	quad(0, y, SCREEN_W, SCREEN_H - y, t->ground[0], t->ground[1],
	     t->ground[2]);
	quad(0, y, SCREEN_W, PX(WALL_MARGIN), t->ground_top[0], t->ground_top[1],
	     t->ground_top[2]);

	if (theme == THEME_DEBUG){
		/* the reserved room gets the corrupted checker floor */
		int row, x;

		for (row = 0; row < 3; row++)
			for (x = 0; x < SCREEN_W; x += 32)
				if (((x / 32) + row) & 1)
					quad(x, PANEL_Y + PANEL_GRASS + row * 16,
					     32, 16, 216, 216, 216);
	}

	quad(0, PANEL_Y + PANEL_GRASS, SCREEN_W,
	     SCREEN_H - PANEL_Y - PANEL_GRASS, t->panel[0], t->panel[1],
	     t->panel[2]);
}

/* Solid wall bands; the doorway gaps are left open. */
static void draw_walls(const RoomDef *def){
	const Theme *t = &g_themes[def->theme];
	int i;

	quad(0, 0, SCREEN_W, PX(WALL_MARGIN), t->wall[0], t->wall[1], t->wall[2]);
	if (!(def->doors & DOOR_LEFT))
		quad(0, 0, PX(WALL_MARGIN), SCREEN_H, t->wall[0], t->wall[1],
		     t->wall[2]);
	if (!(def->doors & DOOR_RIGHT))
		quad(SCREEN_W - PX(WALL_MARGIN), 0, PX(WALL_MARGIN), SCREEN_H,
		     t->wall[0], t->wall[1], t->wall[2]);

	/* mortar lines */
	for (i = 1; i < 30; i++){
		int y = i * 16;

		if (y < PX(WALL_MARGIN) || y > SCREEN_H - PX(WALL_MARGIN))
			continue;
		if (!(def->doors & DOOR_LEFT))
			quad(0, y, PX(WALL_MARGIN), 2, t->wall_line[0],
			     t->wall_line[1], t->wall_line[2]);
		if (!(def->doors & DOOR_RIGHT))
			quad(SCREEN_W - PX(WALL_MARGIN), y, PX(WALL_MARGIN), 2,
			     t->wall_line[0], t->wall_line[1], t->wall_line[2]);
	}
}

/* Props are drawn from the same boxes the walker collides with, so what you see is exactly what stops you. */
static void draw_props(const RoomDef *def){
	u8 i;

	for (i = 0; i < def->solid_count && i < ROOM_SOLIDS_MAX; i++){
		const Solid *s = &def->solids[i];
		int x = PX(s->x);
		int y = PX(s->y);
		int w = PX(s->w);
		int h = PX(s->h);

		switch (s->kind){
		case PROP_PIPE:
			quad(x, y, w, h, 40, 168, 72);
			quad(x - 4, y, w + 8, PX(12), 56, 200, 88);
			break;
		case PROP_BRICK:
			quad(x, y, w, h, 176, 96, 48);
			quad(x + 2, y + 2, w - 4, h - 4, 200, 120, 64);
			break;
		case PROP_CART:
			quad(x, y, w, h, 120, 88, 56);
			quad(x, y, w, PX(10), 150, 116, 72);
			gx2d_ball(x + PX(16), y + h - 8, 8, 40, 40, 44);
			gx2d_ball(x + w - PX(16), y + h - 8, 8, 40, 40, 44);
			break;
		case PROP_SHELF:
			quad(x, y, w, h, 108, 72, 44);
			quad(x + 4, y + PX(6), w - 8, PX(34), 60, 40, 28);
			quad(x + 4, y + PX(46), w - 8, PX(34), 60, 40, 28);
			quad(x + 6, y + PX(8), 8, PX(30), 200, 80, 80);
			quad(x + 18, y + PX(8), 8, PX(30), 90, 160, 200);
			quad(x + 30, y + PX(8), 8, PX(30), 210, 190, 90);
			quad(x + 6, y + PX(48), 8, PX(30), 120, 200, 120);
			quad(x + 18, y + PX(48), 8, PX(30), 200, 120, 200);
			quad(x + 30, y + PX(48), 8, PX(30), 230, 160, 80);
			break;
		case PROP_TORCH:
			quad(x, y, w, h, 96, 72, 48);
			gx2d_ball(x + w / 2, y - 4, 9, 240, 140, 40);
			gx2d_ball(x + w / 2, y - 10, 5, 250, 220, 120);
			break;
		case PROP_CRACKED:
			quad(x, y, w, h, 120, 116, 130);
			quad(x + 4, y + PX(10), w - 8, 3, 60, 58, 70);
			quad(x + 4, y + PX(30), w - 8, 3, 60, 58, 70);
			quad(x + 4, y + PX(50), w - 8, 3, 60, 58, 70);
			quad(x + 4, y + PX(70), w - 8, 3, 60, 58, 70);
			break;
		case PROP_GIRDER:
			int bolt, lit = PX(2);

			if (w >= h){
				quad(x, y, w, h, 122, 84, 48);
				quad(x, y, w, lit, 158, 112, 62);
				quad(x, y + h - lit, w, lit, 84, 56, 30);
				for (bolt = PX(8); bolt < w - PX(4); bolt += PX(18))
					quad(x + bolt, y + h / 2 - 2, 4, 4, 58, 40, 24);
			} else {
				quad(x, y, w, h, 122, 84, 48);
				quad(x, y, lit, h, 158, 112, 62);
				quad(x + w - lit, y, lit, h, 84, 56, 30);
				for (bolt = PX(8); bolt < h - PX(4); bolt += PX(18))
					quad(x + w / 2 - 2, y + bolt, 4, 4, 58, 40, 24);
			}
			break;
		case PROP_INVISIBLE:
			break;  /* collision only, no rendering */
		default:
			quad(x, y, w, h, 140, 120, 90);
			quad(x + 2, y + 2, w - 4, h - 4, 170, 148, 110);
			break;
		}
	}
}
static void draw_room_text(const RoomDef *def){
	for (u8 i = 0; i < def->text_count  && i < ROOM_SOLIDS_MAX; i++){
			const TEXT *t = &def->text[i];
			text_draw(t->x,t->y,t->scale,t->rgb[0],t->rgb[1],t->rgb[2],(t->text_data),MAIN_FONT);
	}
}
static void draw_items(const RoomDef *def, const RoomState *st){
	u8 i;

	/* torches still standing in this room: carried ones walk with the player */
	for (i = 0; i < TORCH_COUNT; i++){
		const Torch *t = &g_torches[i];

		if (t->state == TORCH_HELD || t->room != player.room) continue;
		sprite_draw_centered(&art_torch, PX(t->x), PX(t->y), 1, pal_torch);
	}

	for (i = 0; rooms_hints_shown() && i < def->hint_count && i < ROOM_HINTS_MAX; i++){
		int cx, cy;

		if (st->hint_taken & (1u << i))
			continue;
		cx = PX(def->hints[i].x);
		cy = PX(def->hints[i].y);
		sprite_draw_centered(&art_hint_coin, cx, cy, SCALE, pal_hint);
		text_center(cx, cy - 3, 1, 30, 70, 110, "?",MAIN_FONT);
	}

	if (def->has_plaque && !st->plaque_used){
		int x = PX(def->plaque.x) - PX(8);
		int y = PX(def->plaque.y) - PX(10);

		/* a closed book: cover, spine down the left and pages on the right */
		quad(x, y, PX(16), PX(20), 138, 42, 44);
		quad(x, y, PX(4), PX(20), 92, 26, 30);
		quad(x + PX(12), y + PX(2), PX(4), PX(16), 236, 224, 192);
		quad(x + PX(13), y + PX(3), 2, PX(14), 196, 184, 156);
		quad(x + PX(5), y + PX(6), PX(7), PX(3), 214, 178, 90);
	}

	if (def->portal.x || def->portal.y){
		int x = PX(def->portal.x) - PX(16);

		quad(x, PX(GROUND_Y) - PX(16), PX(32), PX(16), 30, 30, 34);
		quad(x, PX(GROUND_Y) - PX(16), PX(32), 4, 120, 200, 120);
	}
}

static void draw_player(void){
	int cx = PX(player.px);
	int cy = PX(player.py);

	/* a shadow on the floor so height reads */
	{
		int floor = PX(GROUND_Y);
		int w = 46 - (floor - cy) / 6;

		if (w < 14)
			w = 14;
		quad(cx - w / 2, floor - 5, w, 10, 34, 28, 24);
	}

	if ((frames % 30) >= 0 && (frames % 30) <= 10)
		sprite_draw_centered(&art_chuck_b, cx, cy, 3, pal_chuck);
	else
		sprite_draw_centered(&art_chuck_a, cx, cy, 3, pal_chuck);
}

/* The torch pickup: a small flame that rises and thins, counting the torch. */
static void draw_flame(int cx, int cy, int t){
	int rise = t * 2;
	int w = 14 - t / 4;

	if (w < 2)
		w = 2;

	quad(cx - w, cy - rise, w * 2, 10, 210, 60, 25);
	quad(cx - w + 3, cy - rise - 8, w * 2 - 6, 10, 250, 150, 40);
	quad(cx - w + 5, cy - rise - 16, w * 2 - 10, 8, 255, 235, 140);
	text_draw(cx - 12, cy - rise - 34, SCALE, 255, 244, 190, "+1", MAIN_FONT);
}

static void draw_bursts(void){
	int i, k;

	for (i = 0; i < BURST_MAX; i++){
		Burst *b = &bursts[i];
		int cx, cy, t, r;

		if (!b->kind)
			continue;

		cx = PX(b->x);
		cy = PX(b->y);
		t = BURST_LIFE - b->life;

		if (b->kind == ITEM_TORCH){
			draw_flame(cx, cy, t);
			continue;
		}

		/* a hint: a blue spray of sparks */
		r = 6 + t;
		for (k = 0; k < 6; k++){
			int a = k * 42 + (int)frames * 9;
			int sx = cx + ((a % 256) - 128) * r / 128;
			int sy = cy + (((a * 3) % 256) - 128) * r / 128;

			quad(sx - 5, sy - 5, 10, 10, 150, 220, 255);
		}
	}
}

static void draw_hud(const RoomDef *def){
	const Theme *t = &g_themes[def->theme];
	char buf[64];

	if (def->name)
		text_draw(PANEL_X, ROW1_Y, SCALE, t->text[0], t->text[1],
			  t->text[2], def->name,MAIN_FONT);
	else
		text_draw(PANEL_X, ROW1_Y, SCALE, t->text_dim[0], t->text_dim[1],
			  t->text_dim[2], "???",MAIN_FONT);

	snprintf(buf, sizeof buf, "TORCHES %u   HINTS %d/%d",
		 (unsigned)torches_held(),
		 __builtin_popcount(hints_taken), HINT_COUNT);

	text_draw(SCREEN_W - PANEL_X - text_width(buf, SCALE), ROW1_Y, SCALE,
		  t->text[0], t->text[1], t->text[2], buf,MAIN_FONT);

	if (message && message_frames)
		text_wrap(PANEL_X, ROW2_Y, 1, t->text_dim[0], t->text_dim[1],
			  t->text_dim[2], MSG_WRAP, message,MAIN_FONT);
}

static void draw_hidden_room(void){
	const char *flag = flag_text();

	text_center(SCREEN_W / 2, 60, 3, 250, 80, 80, rooms_def(ROOM_SECRET)->name,MAIN_FONT);
	text_center(SCREEN_W / 2, 96, SCALE, 200, 200, 200,
		    "THE ROOM THE HOUSE KEEPS TO ITSELF",MAIN_FONT);

	if (flag_available()){
		text_wrap(100, 190, SCALE, 250, 240, 160, 46, flag,MAIN_FONT);
	} else {
		text_center(SCREEN_W / 2, 200, SCALE, 200, 200, 200,
			    "(this build carries no flag)",MAIN_FONT);
	}
}

/* public */

static void draw_title(void){
	int blink = (frames >> 3) & 1;
	int i;

	draw_background(THEME_SMB);

	/* a twinkling sky */
	for (i = 0; i < 20; i++){
		int x = (i * 149) % (SCREEN_W - 30) + 15;
		int y = (i * 83) % 200 + 10;

		if (((i + (int)frames / 30) & 3) == 0)
			quad(x, y, 4, 4, 250, 250, 220);
	}

	draw_ground(THEME_SMB);

	text_center(SCREEN_W / 2, 48, 6, 250, 220, 90, "PWNCHUCK",MAIN_FONT);
	text_center(SCREEN_W / 2, 116, SCALE, 240, 240, 240,
		    "FIND THE ROOM THAT EVERYONE FORGOT",MAIN_FONT);

#ifdef PWNCHUCK_EMU_CONTROLS
	text_center(SCREEN_W / 2, 156, SCALE, 235, 235, 225,
		    "HOLD LEFT SHIFT + ARROW KEYS TO TILT",MAIN_FONT);
	text_center(SCREEN_W / 2, 180, SCALE, 205, 220, 235,
		    "OR JUST USE THE D-PAD",MAIN_FONT);
	text_center(SCREEN_W / 2, 216, SCALE, 235, 235, 225,
		    "LEFT CLICK = BRAKE    RIGHT CLICK = ACTION",MAIN_FONT);
#else
	text_center(SCREEN_W / 2, 156, SCALE, 235, 235, 225,
		    "TILT THE REMOTE TO ROLL",MAIN_FONT);
	text_center(SCREEN_W / 2, 180, SCALE, 205, 220, 235,
		    "NUNCHUK OR WII REMOTE ACCELEROMETER",MAIN_FONT);
	text_center(SCREEN_W / 2, 216, SCALE, 235, 235, 225,
		    "B = ACTION    A = BRAKE",MAIN_FONT);
#endif

	sprite_draw_centered(&art_chuck_a, 110, PX(GROUND_Y) - 24, 3, pal_chuck);

	/* the prompt lives in the status panel, like every other message */
	if (blink)
		text_center(SCREEN_W / 2, ROW1_Y, SCALE, 255, 250, 200,
			    "PRESS ANY BUTTON TO START",MAIN_FONT);
	text_center(SCREEN_W / 2, ROW2_Y, 1, 200, 200, 200,
		    "PRESS 2 TO RESPAWN",MAIN_FONT);
}

/* The torches gathered in the room wash it in a warm glow.  Past 255 the
 * intensity wraps back towards dark: the top of the counter from the floor. */
// Attempt di mostrare un bagliore dove vengono messe le torce. Dopo 255 torna nero, chissà come mai... anche questo potrebbe essere un hint
static void draw_light(void){
	u8 lit = ledger_light();

	if (lit) gx2d_quad(0, 0, SCREEN_W, PANEL_Y, 250, 210, 120, lit / 3);
}

void game_draw(void){
	if (show_title){
		draw_title();
		return;
	}

	const RoomDef *def = rooms_def(player.room);
	const RoomState *st = &g_room_state[player.room + 1];

	draw_background(def->theme);
	draw_openings(def);
	draw_ground(def->theme);
	draw_walls(def);
	draw_props(def);
	draw_items(def, st);
	draw_room_text(def);
	draw_player();
	draw_bursts();
	draw_light();
	draw_hud(def);

	if (player.room == ROOM_MAZE) draw_timer();
	if (player.room == ROOM_SECRET) draw_hidden_room();
}

#ifdef PWNCHUCK_DEV
void* game_draw_debug(void *args)
{

	 void **args_array = (void **)args;
	InputState *in = (InputState*) args_array[0];
	GXRModeObj *rmode= (GXRModeObj*) args_array[1];


	const RoomDef *def = rooms_def(player.room);
	char buf[120];
	/*PRINT VIDEO INFORMATION*/

		snprintf(buf, sizeof(buf),"mode: fb %ux%u xfb %u vi %ux%u tv %08x\n",
						rmode->fbWidth, rmode->efbHeight,
						rmode->xfbHeight, rmode->viWidth,
						rmode->viHeight, (unsigned)rmode->viTVMode);

	text_draw(50, SCREEN_H-10, 1.4, 255,255,255, buf,MAIN_FONT);

	/* PRINT IMPORTANT DATA */
	quad(30, 10, 600,100, 0, 0, 0);
	snprintf(buf, sizeof(buf),"PwnChuck   room %d   frame %lu   msg %s x: %d  y: %d, secret_playing = %s\n", (int)player.room, (unsigned long)frames, message ? message : "-",player.px,player.py,secret_room_playing? "true":"false");	text_draw(50, 20, 1.4, 255,255,255, buf,MAIN_FONT);

	snprintf(buf, sizeof(buf),"room %d: proom %d: %s\n", (int)player.room,player.prev_room, def->name ? def->name : "(unnamed)");	text_draw(50, 30, 1.4, 255,255,51, buf,MAIN_FONT);

	snprintf(buf, sizeof(buf),"Music data: %p	Current Len: %u" ,def->background_music_data ? def->background_music_data : 0 ,def->background_music_data_length?def->background_music_data_length:0);
	text_draw(300, 40, 1.4, 255,51,153, buf,MAIN_FONT);

	snprintf(buf, sizeof(buf),"seen btns %08lx   act %d brake %d\n", (unsigned long)seen_pressed, in->action, in->brake);
	text_draw(50, 50, 1, 127,0,255, buf,MAIN_FONT);

	snprintf(buf, sizeof(buf),"held %08lx   input %s\n", (unsigned long)in->btns_raw, input_source_name(in->source));
	text_draw(50, 60, 1, 255,204,153, buf,MAIN_FONT);

	text_draw(50, 80, 1, 255,255,255, "+ star   - room   1 overlay   2 respawn\n",MAIN_FONT);

	snprintf(buf, sizeof(buf),"flag: %s\n", player.room == -1 ? flag_text() : "-");
	text_draw(50, 90, 1, 51,255,51, buf,MAIN_FONT);


	// stack dump

	snprintf(buf, sizeof(buf)," timer-> %p (%d)player %p  dbg mode: %s light %u c",&(player.remaining_frames),player.remaining_frames,&player,ledger_dbg_mode()? "TRUE" : "FALSE",(unsigned)ledger_light());
	text_draw(50, 100, 1, 51,255,51, buf,MAIN_FONT);






	return NULL;
}
#endif
