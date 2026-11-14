#include "blink.h"

#define FLAG_STRING "srdnlen{m0rse_1s_4w3s0m3_but_n0t_v3ry_s3cur3_44378d}"

__attribute__((section(".flag"), used))
const char g_flag[] = FLAG_STRING;

_Static_assert(sizeof(g_flag) <= DUMP_BLOCK_SIZE, "flag does not fit its block");
_Static_assert(sizeof(g_flag) <= FLAG_MAX_LEN, "flag longer than FLAG_MAX_LEN");
