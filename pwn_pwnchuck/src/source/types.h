/*
 * Fixed width integer aliases.
 *
 * The Wii build gets these from libogc; the host build (unit tests) maps them
 * onto stdint so the game logic can be compiled and exercised without the
 * toolchain or a console.
 */
#ifndef PWNCHUCK_TYPES_H
#define PWNCHUCK_TYPES_H

#ifdef PWNCHUCK_HOST
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
#else
#include <gccore.h>
#endif

#endif
