#include <malloc.h>

#include "gx2d.h"

#define GX2D_FIFO_SIZE (256 * 1024)

static void *s_fifo;

void gx2d_init(GXRModeObj *rmode){
	Mtx44 proj;
	GXColor clear = { 0, 0, 0, 0xff };

	s_fifo = memalign(32, GX2D_FIFO_SIZE);
	GX_Init(s_fifo, GX2D_FIFO_SIZE);
	GX_SetCopyClear(clear, 0x00ffffff);

	/* orthographic projection over a 640x480 top-left origin space */
	guOrtho(proj, 0, GX2D_SCREEN_H, 0, GX2D_SCREEN_W, 0, 300);
	GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);

	GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
	GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	GX_SetDispCopyGamma(GX_GM_1_0);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAPNULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

	/* flat 2D: no depth test, straight alpha blending, painter's order */
	GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
}

void gx2d_quad(int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a){
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position2s16(x, y);
	GX_Color4u8(r, g, b, a);
	GX_Position2s16(x + w, y);
	GX_Color4u8(r, g, b, a);
	GX_Position2s16(x + w, y + h);
	GX_Color4u8(r, g, b, a);
	GX_Position2s16(x, y + h);
	GX_Color4u8(r, g, b, a);
	GX_End();
}

void gx2d_ball(int cx, int cy, int radius, u8 r, u8 g, u8 b){
	const int outer = radius;
	const int mid = radius * 3 / 4;
	const int inner = radius / 2;

	/* cheap round-ish blob: a cross plus an inner highlight */
	gx2d_quad(cx - mid, cy - outer, mid * 2, outer * 2,
		r * 3 / 4, g * 3 / 4, b * 3 / 4, 0xff);
	gx2d_quad(cx - outer, cy - mid, outer * 2, mid * 2,
		r * 3 / 4, g * 3 / 4, b * 3 / 4, 0xff);
	gx2d_quad(cx - inner, cy - inner, inner * 2, inner * 2, r, g, b, 0xff);
}
