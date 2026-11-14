/*
 * The flag payload.
 *
 * The bytes in flag_data.c are the flag text, generated at build time by
 * tools/mkflag.py.  Only the station build carries them; the player build
 * ships the same table zeroed with g_flag_len == 0.  The generator writes the
 * table back to zeroed after the build, so the plaintext does not persist in
 * the source tree.
 */
#include "flag.h"
#include "flag_data.h"

static char s_text[FLAG_MAX];
static bool s_ready;

bool flag_available(void){
	return g_flag_len > 0;
}

const char *flag_text(void){
	u32 len = g_flag_len;
	u32 i;

	if (s_ready)
		return s_text;

	if (len > FLAG_MAX - 1)
		len = FLAG_MAX - 1;

	for (i = 0; i < len; i++)
		s_text[i] = (char)g_flag_payload[i];
	s_text[len] = '\0';
	s_ready = true;

	return s_text;
}

void flag_wipe(void){
	u32 i;

	for (i = 0; i < FLAG_MAX; i++)
		s_text[i] = 0;
	s_ready = false;
}