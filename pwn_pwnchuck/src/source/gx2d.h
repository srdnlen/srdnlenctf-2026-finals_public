/*
 * gx2d - minimal untextured 2D renderer.
 *
 * Logical space is 640x480 with the origin at the top-left and y growing
 * downwards (same convention as the libogc console). All drawing happens
 * inside the current GX frame; the caller presents via main.c.
 */
#ifndef PWNCHUCK_GX2D_H
#define PWNCHUCK_GX2D_H

#include <gccore.h>

// Logical 2D space; matches the 4:3 640x480 output.
#define GX2D_SCREEN_W 640
#define GX2D_SCREEN_H 480

void gx2d_init(GXRModeObj *rmode);
void gx2d_quad(int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a);
void gx2d_ball(int cx, int cy, int radius, u8 r, u8 g, u8 b);

#endif