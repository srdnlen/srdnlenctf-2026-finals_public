#include <stdlib.h>
#include <string.h>
#include "input.h"
#include <wiiuse/wpad.h>
#include <math.h> 


s32 channel;		         // The wiimote channel in a from of an integer
s32 rumble_frames;          // The frames left to the rumble 
RumbleIntensity intensity; // The intensity of the vibration


s16 tilt_axis_degrees(f32 angle_deg) {
    f32 abs_angle = fabs(angle_deg);

    if (abs_angle <= TILT_DEADZONE) return 0;

    else if (abs_angle > TILT_FULL) abs_angle = TILT_FULL;

    f32 active_range = TILT_FULL - TILT_DEADZONE;
    
    f32 active_tilt = abs_angle - TILT_DEADZONE;

    s16 result = (s16)((active_tilt / active_range) * 256.0f)*SPEED_COEF;

    return (angle_deg < 0) ? -result : result;
}
void input_init(InputState *in){
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    
	u32 exp_type;
    s32 probe_result;

    // Continua a ciclare finché il Wiimote sul canale 0 non risponde correttamente
    while (1) {
        WPAD_ScanPads();
        
        probe_result = WPAD_Probe(WPAD_CHAN_0, &exp_type);
        
        if (probe_result == WPAD_ERR_NONE) {
            break; // Connessione stabilita con successo!
        }

        
        VIDEO_WaitVSync(); 
    }
}

void input_poll(InputState *in){
	WPADData *d = NULL;
	u32 held;
	u32 pressed;

	memset(in, 0, sizeof(*in));
	
	WPAD_ScanPads();

	d = WPAD_Data(WPAD_CHAN_0);
	
	if (d == NULL)
		return;

	held = d->btns_h;
	pressed = d->btns_d;

	in->data_present = d->data_present;
	in->exp_type = (u8)d->exp.type;
	in->battery = d->battery_level;
	in->btns_raw = held;
	in->wiimote_ok = (d->err == WPAD_ERR_NONE);

	#ifdef PWNCHUCK_DEV
		in->toggle_debug = (pressed & WPAD_BUTTON_1) != 0;
	#endif
	in->reset_requested = (pressed & WPAD_BUTTON_2) != 0;
	in->btns_pressed = pressed;

	if (in->wiimote_ok){
        in->source = TILT_SRC_WIIMOTE;
        
        f32 roll = d->orient.roll;
        f32 pitch = d->orient.pitch;
        
        in->tilt_x = tilt_axis_degrees(roll);
        in->tilt_y = -tilt_axis_degrees(pitch);
        
        in->action = (held & WPAD_BUTTON_B) != 0;
        in->brake  = (held & WPAD_BUTTON_A) != 0;
    } else {
		in->source = TILT_SRC_NONE;
		return;
	}

	#ifdef PWNCHUCK_EMU_CONTROLS
		// Enable D-pad only on the Emulator build to make it easier to test tilt controls without a real Wiimote
		if (in->tilt_x == 0 && in->tilt_y == 0){
			s16 dx = 0, dy = 0;

			if (held & WPAD_BUTTON_LEFT)        dx = -256;
			else if (held & WPAD_BUTTON_RIGHT)  dx = 256;

			if (held & WPAD_BUTTON_UP)          dy = 256;
			else if (held & WPAD_BUTTON_DOWN)   dy = -256;

			if (dx || dy){
				in->tilt_x = dx;
				in->tilt_y = dy;
				in->source = TILT_SRC_DPAD;
			}
		}
	#endif
}



void update_vibration(){
	
	if(rumble_frames > 0 ){ 	// Checks if the controller should vibrate
		bool status;			// Variable needed to set the vibration status
		switch (intensity)		// Check the intensity
		{
			case RUMBLE_STRONG: // If it is STRONG
				status = (rumble_frames % RUMBLE_FRAMES_STRONG == 0); // Sets the status to true every frame
				break;
			case RUMBLE_MEDIUM: // If it is MEDIUM
				status = (rumble_frames % RUMBLE_FRAMES_MEDIUM == 0); // Sets the status to true every three frames
				break;
			case RUMBLE_LOW:	// If it is LOW
				status = (rumble_frames % RUMBLE_FRAMES_LOW == 0);	 // Sets the status to true every  six frames
				break;

			case RUMBLE_NONE:  // If it is none or the other things it stops
			default:
				status = false;
			break;
		}

		
		rumble_frames --; 	  //decrements the avaiable frames 
		WPAD_Rumble(channel,status); // Sets the rumble
	}else{
		/** Resets the data **/
		WPAD_Rumble(channel,false);
		rumble_frames = 0;
		intensity = RUMBLE_NONE;
		channel = -1;

	}
}

void set_rumble(s32 chan,s32 frames,RumbleIntensity vib){
	rumble_frames = frames;
	intensity = vib;
	channel = chan;
	update_vibration();
}

#ifdef PWNCHUCK_DEV
const char *input_source_name(TiltSource src){
	switch (src){
    	case TILT_SRC_NUNCHUK:  return "nunchuk";
    	case TILT_SRC_WIIMOTE:  return "wiimote";
    	case TILT_SRC_DPAD:     return "dpad";
    	default:                return "none";
	}
}
#endif
