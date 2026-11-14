/*
 * Tilt input abstraction.
 *
 * The nunchuk accelerometer is the intended source; when no nunchuk is
 * attached (or the extension is not visible, as happens with some emulator
 * configurations) the Wiimote accelerometer is used instead. Both are
 * normalised into a screen-space tilt vector in Q8 fixed point.
 */
#ifndef PWNCHUCK_INPUT_H
#define PWNCHUCK_INPUT_H

#include <stdbool.h>

#include "types.h"

#define TILT_FULL 90    	// tilt in degrees
#define TILT_DEADZONE 15 	// ignore deflections below ~15 degrees
#define REST_LEARN_FRAMES 30 	// frames of boot-time averaging used to learn the rest pose
#define SPEED_COEF 1.5

#define RUMBLE_FRAMES_STRONG 1  // The number of frames that the motor stops for if it is STRONG
#define RUMBLE_FRAMES_MEDIUM 3  // The number of frames that the motor stops for if it is MEDIUM
#define RUMBLE_FRAMES_LOW 6	    // The number of frames that the motor stops for if it is LOW	

typedef enum {
	TILT_SRC_NONE = 0,
	TILT_SRC_NUNCHUK,
	TILT_SRC_WIIMOTE,
	TILT_SRC_SIM,       // scripted source used by the logic tests
	TILT_SRC_DPAD,      // emulator build: the D-pad stands in for the tilt
} TiltSource;

typedef struct {
	TiltSource source;
	bool wiimote_ok;
	u16 data_present;   // raw WPAD data_present bits (debug)
	u8 exp_type;        // raw extension type (debug)
	u8 battery;
	u32 btns_raw;       // merged button bitmask (debug)

	// calibrated tilt, Q8 fixed point (-256..+256 = full deflection)
	s16 tilt_x, tilt_y;

	// raw accelerometer of the active source plus the learned rest pose
	u16 raw_x, raw_y, raw_z;
	s16 rest_x, rest_y;

	// action buttons
	bool action;            // nunchuk Z / wiimote B
	bool brake;             // nunchuk C / wiimote A
	bool toggle_debug;      // button 1
	u32 btns_pressed;       // buttons that went down this frame
	bool reset_requested;   // button 2: back to the first room
} InputState;

typedef enum {
	RUMBLE_NONE,
	RUMBLE_STRONG,
	RUMBLE_MEDIUM,
	RUMBLE_LOW,
} RumbleIntensity;

void set_rumble(s32 chan,s32 frames,RumbleIntensity vib); //funzione che setta tutti i parametri necessari per attivare la vibrazione
void update_vibration(); //funzione che controlla se e come far vibrare il controller

void input_init(InputState *in);
void input_poll(InputState *in);
s16 tilt_axis_degrees(f32 angle_deg);

#ifdef PWNCHUCK_DEV
const char *input_source_name(TiltSource src);
#endif

#endif
