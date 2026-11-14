/*
 * PwnChuck - venue hardware probe (tools/hwprobe)
 *
 * Purpose-built diagnostic that is run on the real Wii before the event to
 * confirm that the controllers behave the way the game assumes.  It mirrors
 * the input rules of source/input.c exactly:
 *
 *   - WPAD_Init() + WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR)
 *   - the nunchuk accelerometer is preferred when it is usable, otherwise
 *     the Wiimote accelerometer is used
 *   - a rest pose is learned over the first 30 usable frames and slowly
 *     re-centered, then tilt = (raw - rest) * 256 / 300 clamped to +-256,
 *     with a deadzone of 28 raw counts
 *   - tilt_x > 0 means "right", tilt_y > 0 means "up" (screen space)
 *
 * The screen shows, live:
 *   - Wiimote: raw accel X/Y/Z, rest pose, derived tilt, buttons held,
 *     battery, and which sections the current report carries
 *     (buttons/accel/expansion/IR)
 *   - extension: detected type (none/nunchuk/classic/motion plus/...), and
 *     for a nunchuk its raw accel, rest, tilt, stick and C/Z buttons
 *   - a "flip" checklist showing which sign each tilt direction must have
 *
 * Controls:
 *   2     - relearn the rest pose (hold the controller still at rest first)
 *   HOME  - exit
 *
 * Build (from the repository root, devkitPPC image):
 *   docker run --rm -v "$PWD":/project -w /project \
 *       devkitpro/devkitppc:latest make -C tools/hwprobe
 *
 * Single file, C99, libogc only.  See README.md for the venue checklist.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <gccore.h>
#include <ogc/console.h>
#include <wiiuse/wpad.h>

/* Console inset in pixels: real TVs crop a few percent (overscan), so keep
 * the text away from the edges.  20 px matches the libogc console example. */
#define WINDOW_X 20
#define WINDOW_Y 20

/* Game rules - keep in sync with source/input.c. */
#define REST_LEARN_FRAMES 30                 /* samples averaged for rest  */
#define TILT_DEADZONE     28                 /* raw counts treated as zero */
#define TILT_SCALE_NUM    256
#define TILT_SCALE_DEN    300
#define TILT_LIMIT        256                /* game clamps to +-256       */

/* A sensor that stays missing this long gets its rest pose relearned, so a
 * controller swap at the venue never inherits the old controller's rest. */
#define RELEARN_AFTER     60

static GXRModeObj *rmode;
static void *frameBuffer[2];
static int draw_buffer;

typedef struct {
	u16 x, y, z;            /* latest raw accel, 10 bit, ~512 = 0 g      */
	s16 rest_x, rest_y;     /* learned rest pose                         */
	s16 tilt_x, tilt_y;     /* game tilt, -256..256, +x right / +y up    */
	s32 accum_x, accum_y;
	u16 samples;            /* rest samples collected so far             */
	int absent;             /* consecutive frames without this sensor    */
	bool present;
} sensor_state;

static sensor_state remote = { .rest_x = 512, .rest_y = 512 };
static sensor_state nunchuk = { .rest_x = 512, .rest_y = 512 };

/* Per-frame snapshot of the WPAD data, used by the renderer. */
typedef struct {
	bool data_ok;           /* WPAD_Data() returned usable data       */
	bool wiimote_ok;        /* game would accept the Wiimote accel    */
	bool nunchuk_ok;        /* game would accept the nunchuk accel    */
	s32 err;                /* d->err (WPAD_ERR_*)                    */
	u32 report;             /* d->data_present bits                   */
	u8 battery;             /* raw battery status byte                */
	int expansion_type;     /* EXP_* code, -1 when there is no data   */
	u32 buttons;            /* low 16 bits of btns_h (Wiimote)        */
	u32 nc_buttons;         /* WPAD nunchuk button bits (Z/C)         */
	u16 nc_raw_buttons;     /* raw nunchuk button bits                */
	u8 stick_x, stick_y;
	const char *source;     /* "WIIMOTE", "NUNCHUK" or "nothing"      */
	bool exit_requested;
} pad_state;

static pad_state pad;

/* ---------------- game math (mirrors source/input.c) ------------------ */

static s16 tilt_axis(int raw, int rest){
	int d = raw - rest;

	if (abs(d) < TILT_DEADZONE)
		d = 0;

	d = d * TILT_SCALE_NUM / TILT_SCALE_DEN;

	if (d > TILT_LIMIT)
		d = TILT_LIMIT;
	if (d < -TILT_LIMIT)
		d = -TILT_LIMIT;

	return (s16)d;
}

static void sensor_relearn(sensor_state *s){
	s->rest_x = 512;
	s->rest_y = 512;
	s->accum_x = 0;
	s->accum_y = 0;
	s->samples = 0;
}

static void sensor_update(sensor_state *s, bool present, u16 x, u16 y, u16 z){
	if (!present){
		s->present = false;
		if (s->absent == RELEARN_AFTER)
			sensor_relearn(s);
		s->absent++;
		return;
	}

	s->present = true;
	s->absent = 0;
	s->x = x;
	s->y = y;
	s->z = z;

	if (s->samples < REST_LEARN_FRAMES){
		s->accum_x += x;
		s->accum_y += y;
		s->samples++;
		if (s->samples >= REST_LEARN_FRAMES){
			s->rest_x = (s16)(s->accum_x / s->samples);
			s->rest_y = (s16)(s->accum_y / s->samples);
		}
	} else {
		if (abs((int)x - s->rest_x) < TILT_DEADZONE)
			s->rest_x += ((int)x - s->rest_x) >> 7;
		if (abs((int)y - s->rest_y) < TILT_DEADZONE)
			s->rest_y += ((int)y - s->rest_y) >> 7;
	}

	s->tilt_x = tilt_axis(x, s->rest_x);
	s->tilt_y = tilt_axis(y, s->rest_y);
}

/* ---------------- wording helpers ------------------------------------- */

static const char *yes_no(int value){
	return value ? "yes" : "no";
}

static const char *tilt_word_x(int v){
	if (v > 0)
		return "RIGHT";
	if (v < 0)
		return "LEFT";
	return "center";
}

static const char *tilt_word_y(int v){
	if (v > 0)
		return "UP";
	if (v < 0)
		return "DOWN";
	return "level";
}

static const char *extension_name(int type){
	switch (type){
	case EXP_NONE:          return "none";
	case EXP_NUNCHUK:       return "nunchuk";
	case EXP_CLASSIC:       return "classic controller";
	case EXP_GUITAR_HERO_3: return "guitar hero 3";
	case EXP_WII_BOARD:     return "balance board";
	case EXP_MOTION_PLUS:   return "motion plus";
	default:                return "unknown";
	}
}

static const char *wpad_error_name(s32 err){
	switch (err){
	case WPAD_ERR_NONE:           return "OK";
	case WPAD_ERR_NO_CONTROLLER:  return "no controller";
	case WPAD_ERR_NOT_READY:      return "not ready";
	case WPAD_ERR_TRANSFER:       return "transfer error";
	case WPAD_ERR_NONEREGISTERED: return "not registered";
	case WPAD_ERR_UNKNOWN:        return "unknown";
	case WPAD_ERR_BAD_CHANNEL:    return "bad channel";
	case WPAD_ERR_QUEUE_EMPTY:    return "queue empty";
	case WPAD_ERR_BADVALUE:       return "bad value";
	case WPAD_ERR_BADCONF:        return "bad config";
	default:                      return "error";
	}
}

/* WPAD_GetStatus() returns the WPAD subsystem state in this libogc, not a
 * per-controller status (that is d->err). */
static const char *wpad_state_name(int state){
	switch (state){
	case WPAD_STATE_DISABLED: return "disabled";
	case WPAD_STATE_ENABLING: return "enabling";
	case WPAD_STATE_ENABLED:  return "enabled";
	default:                  return "unknown";
	}
}

static const struct {
	u32 bit;
	const char *name;
} wiimote_buttons[] = {
	{ WPAD_BUTTON_LEFT,  "Left"  },
	{ WPAD_BUTTON_RIGHT, "Right" },
	{ WPAD_BUTTON_DOWN,  "Down"  },
	{ WPAD_BUTTON_UP,    "Up"    },
	{ WPAD_BUTTON_PLUS,  "+"     },
	{ WPAD_BUTTON_MINUS, "-"     },
	{ WPAD_BUTTON_HOME,  "HOME"  },
	{ WPAD_BUTTON_A,     "A"     },
	{ WPAD_BUTTON_B,     "B"     },
	{ WPAD_BUTTON_1,     "1"     },
	{ WPAD_BUTTON_2,     "2"     },
};

static void text_append(char *buf, size_t cap, const char *word){
	size_t used = strlen(buf);
	size_t add = strlen(word);

	if (used + (used ? 1 : 0) + add + 1 > cap)
		return;
	if (used)
		buf[used++] = ' ';
	memcpy(buf + used, word, add + 1);
}

static void format_held_buttons(u32 held, char *buf, size_t cap){
	size_t i;

	buf[0] = '\0';
	for (i = 0; i < sizeof(wiimote_buttons) / sizeof(wiimote_buttons[0]); i++)
		if (held & wiimote_buttons[i].bit)
			text_append(buf, cap, wiimote_buttons[i].name);
	if (buf[0] == '\0')
		text_append(buf, cap, "none");
}

static void battery_bar(unsigned percent, char *out){
	int i;

	out[0] = '[';
	for (i = 0; i < 10; i++)
		out[1 + i] = (i < (int)(percent / 10)) ? '#' : '.';
	out[11] = ']';
	out[12] = '\0';
}

/* ---------------- input ------------------------------------------------ */

static void pad_update(void){
	WPADData *d;
	bool data_ok, wm_ok, nc_ok;

	WPAD_ScanPads();
	d = WPAD_Data(WPAD_CHAN_0);

	data_ok = (d != NULL) && (d->err == WPAD_ERR_NONE);
	wm_ok = data_ok && (d->data_present & WPAD_DATA_ACCEL);
	nc_ok = wm_ok && (d->data_present & WPAD_DATA_EXPANSION) &&
	        (d->exp.type == EXP_NUNCHUK);

	pad.data_ok = data_ok;
	pad.wiimote_ok = wm_ok;
	pad.nunchuk_ok = nc_ok;
	pad.err = (d != NULL) ? d->err : WPAD_ERR_NO_CONTROLLER;
	pad.report = data_ok ? d->data_present : 0;
	pad.battery = data_ok ? d->battery_level : 0;
	pad.expansion_type = data_ok ? d->exp.type : -1;
	pad.buttons = data_ok ? (d->btns_h & 0xffff) : 0;
	pad.nc_buttons = data_ok ? (d->btns_h &
	                            (WPAD_NUNCHUK_BUTTON_Z | WPAD_NUNCHUK_BUTTON_C)) : 0;
	pad.nc_raw_buttons = data_ok ? d->exp.nunchuk.btns_held : 0;
	pad.stick_x = data_ok ? d->exp.nunchuk.js.pos.x : 0;
	pad.stick_y = data_ok ? d->exp.nunchuk.js.pos.y : 0;

	if (nc_ok)
		pad.source = "NUNCHUK";
	else if (wm_ok)
		pad.source = "WIIMOTE";
	else
		pad.source = "nothing";

	if (data_ok && (d->btns_d & WPAD_BUTTON_2)){
		sensor_relearn(&remote);
		sensor_relearn(&nunchuk);
	}

	pad.exit_requested = data_ok && (d->btns_d & WPAD_BUTTON_HOME);

	sensor_update(&remote, wm_ok,
	              d ? d->accel.x : 0, d ? d->accel.y : 0, d ? d->accel.z : 0);
	sensor_update(&nunchuk, nc_ok,
	              d ? d->exp.nunchuk.accel.x : 0,
	              d ? d->exp.nunchuk.accel.y : 0,
	              d ? d->exp.nunchuk.accel.z : 0);
}

/* ---------------- rendering ------------------------------------------- */

static void render_sensor_values(const sensor_state *s){
	printf("  raw   X %4u   Y %4u   Z %4u\n",
	       (unsigned)s->x, (unsigned)s->y, (unsigned)s->z);
	printf("  rest  X %4d   Y %4d     %s %u/%u\n",
	       (int)s->rest_x, (int)s->rest_y,
	       (s->samples < REST_LEARN_FRAMES) ? "learning" : "learned",
	       (unsigned)s->samples, (unsigned)REST_LEARN_FRAMES);
}

static void render(u32 frame){
	char held[96];
	char bar[16];
	unsigned battery_pct = ((unsigned)pad.battery * 100u + 127u) / 255u;
	int wpad_state = (int)WPAD_GetStatus();

	battery_bar(battery_pct, bar);
	format_held_buttons(pad.buttons, held, sizeof(held));

	printf("PwnChuck venue hardware probe       frame %lu\n",
	       (unsigned long)frame);
	printf("wiimote err %d (%s)   WPAD_GetStatus %d (%s)   format BTNS_ACC_IR\n",
	       (int)pad.err, wpad_error_name(pad.err),
	       wpad_state, wpad_state_name(wpad_state));
	printf("report 0x%02x  [buttons %s] [accel %s] [expansion %s] [ir %s]\n",
	       (unsigned)pad.report,
	       yes_no(pad.report & WPAD_DATA_BUTTONS),
	       yes_no(pad.report & WPAD_DATA_ACCEL),
	       (pad.report & WPAD_DATA_EXPANSION) ? "YES" : "no",
	       yes_no(pad.report & WPAD_DATA_IR));
	printf("battery %u/255 (~%u%%)   %s     dropped %u\n",
	       (unsigned)pad.battery, battery_pct, bar,
	       (unsigned)WPAD_DroppedEvents(WPAD_CHAN_0));

	printf("\n");
	printf("--- WIIMOTE (game uses this only when no nunchuk) ----------\n");
	render_sensor_values(&remote);
	printf("  tilt  X %+4d (%s)   Y %+4d (%s)   [game scale -256..256]\n",
	       (int)remote.tilt_x, tilt_word_x(remote.tilt_x),
	       (int)remote.tilt_y, tilt_word_y(remote.tilt_y));
	printf("  held  0x%04x   %s      A = brake   B = action\n",
	       (unsigned)pad.buttons, held);

	printf("\n");
	printf("--- EXTENSION ----------------------------------------------\n");
	if (pad.data_ok)
		printf("  type  %s (code %d)", extension_name(pad.expansion_type),
		       pad.expansion_type);
	else
		printf("  type  --- (no WPAD data)");
	printf("      >>> GAME USES: %s\n", pad.source);
	if (nunchuk.present){
		render_sensor_values(&nunchuk);
		printf("  tilt  X %+4d (%s)   Y %+4d (%s)\n",
		       (int)nunchuk.tilt_x, tilt_word_x(nunchuk.tilt_x),
		       (int)nunchuk.tilt_y, tilt_word_y(nunchuk.tilt_y));
		printf("  stick X %3u   Y %3u      Z(action) %s   C(brake) %s   raw 0x%02x\n",
		       (unsigned)pad.stick_x, (unsigned)pad.stick_y,
		       (pad.nc_buttons & WPAD_NUNCHUK_BUTTON_Z) ? "ON" : "off",
		       (pad.nc_buttons & WPAD_NUNCHUK_BUTTON_C) ? "ON" : "off",
		       (unsigned)(pad.nc_raw_buttons & 0xff));
	} else {
		printf("  raw   ---    no nunchuk accelerometer data\n");
		printf("  rest  ---\n");
		printf("  tilt  ---    the game cannot read the nunchuk\n");
		printf("  stick ---\n");
	}

	if (pad.expansion_type == EXP_MOTION_PLUS)
		printf("  note: game never enables MP; a nunchuk behind MP may not be read\n");
	else
		printf("\n");

	printf("--- FLIP CHECK (the signs the game expects) -----------------\n");
	printf("  tilt_x > 0 = RIGHT          tilt_y > 0 = UP\n");
	printf("  Hold the controller the way you play, then tilt it \"forward\" - away\n");
	printf("  from you, the way that should walk the player UP the screen. The Y\n");
	printf("  number of the sensor marked GAME USES must go POSITIVE. Tilt it\n");
	printf("  right: X must go POSITIVE. If a sign is inverted, record it.\n");
	printf("  2 = relearn rest pose      HOME = exit\n");
	if (!remote.present && !nunchuk.present)
		printf("  waiting for a controller: press A, or the red SYNC button\n");
}

/* ---------------- video ------------------------------------------------ */

static void video_init(void){
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);

	/* console_init() clears the whole framebuffer and resets the cursor,
	 * so painting both buffers here keeps the first flip clean. */
	console_init(frameBuffer[0], WINDOW_X, WINDOW_Y,
	             rmode->fbWidth, rmode->xfbHeight,
	             rmode->fbWidth * VI_DISPLAY_PIX_SZ);
	console_init(frameBuffer[1], WINDOW_X, WINDOW_Y,
	             rmode->fbWidth, rmode->xfbHeight,
	             rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	VIDEO_SetNextFramebuffer(frameBuffer[0]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}

int main(void){
	u32 frame = 0;

	video_init();

	WPAD_Init();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);

	while (SYS_MainLoop()){
		pad_update();
		if (pad.exit_requested)
			break;

		/* Draw into the buffer that is not being scanned out right
		 * now, then hand it to the VI - no tearing, no flicker. */
		draw_buffer ^= 1;
		console_init(frameBuffer[draw_buffer], WINDOW_X, WINDOW_Y,
		             rmode->fbWidth, rmode->xfbHeight,
		             rmode->fbWidth * VI_DISPLAY_PIX_SZ);
		render(frame);

		VIDEO_SetNextFramebuffer(frameBuffer[draw_buffer]);
		VIDEO_Flush();
		VIDEO_WaitVSync();

		frame++;
	}

	WPAD_Shutdown();

	return 0;
}
