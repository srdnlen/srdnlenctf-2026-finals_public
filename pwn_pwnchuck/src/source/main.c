/*
 * Boot, video setup and the fixed step main loop.  All game logic lives in
 * game.c / physics.c, all input handling in input.c.
*/
#include <asndlib.h>
#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include "game.h"
#include "gx2d.h"
#include "input.h"
#include "./audio/menu_audio.h"


static int whichfb = 0;
static GXRModeObj *rmode = NULL;
static void *frame_buffer[2] = { NULL, NULL };

static void video_init(void)
{
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	frame_buffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frame_buffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// potrebbe non servire, ma così se ci sono errori scrive da qualche parte
	console_init(frame_buffer[0], 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frame_buffer[0]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync(); 
	if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

/*Gestione del reset*/
bool reset_requested = false;
void reset_pressed_isr(u32 irq, void *ctx) {reset_requested = true;}

int main(int argc, char **argv){
	InputState in;		      // The variable that contains the usefull controller data polled each frame
	srand(SYS_Time());       // Initializzes the seed for the rand() function
	SYS_SetResetCallback(reset_pressed_isr);
	/** First initlializzation **/
	video_init();		  // Initializzes the frame buffers and configures the GPU
	ASND_Init();	     // Initializzes the sound library and sets the hardware sample rate to 48000khz
	gx2d_init(rmode);   // Finishes the GPU setup setting up the variables needed to make it draw 2d objects
	input_init(&in);   // Initializzes the controller, sets up the input structure and configures the wiimote to use the accelerometer 
	game_init();	  // Stops the music, sets up the vuln and instantiates the rooms.
	ASND_Pause(0);
	ASND_SetInfiniteVoice(0, VOICE_STEREO_16BIT, 25201, 0,(void*)&menu_raw, menu_raw_len, 255, 255);
	/** Main loop **/
	while (SYS_MainLoop()){  
		input_poll(&in);		    //I read the state of the controller
		
		#ifdef PWNCHUCK_DEV	
			/*Checks if it is the dev build, then if you press 1 the game loads a gui*/
			if (in.toggle_debug) game_toggle_debug();
		#endif

		game_update(&in); 	  // Handle the game logic
		game_draw();		 // Put the updated frame to the buffer
		update_vibration(); // Update the vibration motor status 
		
							
		#ifdef PWNCHUCK_DEV
			/* Checks if it the dev branch then if it is and if the debug is enabled it shows the debug menu*/
			if (game_debug_enabled() ){
						void **targs = malloc(sizeof(void *) * 2); //allocates the arguments ()
						targs[0] = &in;
						targs[1] =  rmode;
						game_draw_debug(targs);
				}
		#endif
		GX_DrawDone();
		GX_CopyDisp(frame_buffer[whichfb], GX_TRUE);
		GX_Flush();

		VIDEO_SetNextFramebuffer(frame_buffer[whichfb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		whichfb ^= 1;
		if(reset_requested) break;
	}

	WPAD_Shutdown();
	return 0;
}
