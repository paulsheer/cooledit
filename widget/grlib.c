/* grlib.c - skeleton svgalib
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>

/* this is a stripped version of gl from svgalib-1.2.10
just to provide dummy setcontext, getcontext
and clip functions. It does nothing else really. */

/* It bares a vague resemblance to : */

/* Framebuffer Graphics Libary for Linux, Copyright 1993 Harm Hanemaayer */
/* grlib.c      Main module */


#include <stdlib.h>
#include <my_string.h>
#include "vgagl.h"


#define uchar unsigned char
#define swap(x, y) { int temp = x; x = y; y = temp; }
#define swapb(x, y) { uchar temp = x; x = y; y = temp; }
#define max(x, y) ((x > y) ? x : y)
#define min(x, y) ((x > y) ? y : x)
#define outside(x, y) (x < __clipx1 || x > __clipx2 || y < __clipy1 \
	|| y > __clipy2)
#define x_outside(x) (x < __clipx1 || x > __clipx2)
#define y_outside(y) (y < __clipy1 || y > __clipy2)
#define clipxleft(x) if (x < __clipx1) x = __clipx1;
#define clipxright(x) if (x > __clipx2) x = __clipx2;
#define clipytop(y) if (y < __clipy1) y = __clipy1;
#define clipybottom(y) if (y > __clipy2) y = __clipy2;


#define setpixel (*(__currentcontext.ff.driver_setpixel_func))
#define getpixel (*(__currentcontext.ff.driver_getpixel_func))
#define hline (*(__currentcontext.ff.driver_hline_func))
#define fillbox (*(__currentcontext.ff.driver_fillbox_func))
#define putbox (*(__currentcontext.ff.driver_putbox_func))
#define getbox (*(__currentcontext.ff.driver_getbox_func))
#define putboxmask (*(__currentcontext.ff.driver_putboxmask_func))
#define putboxpart (*(__currentcontext.ff.driver_putboxpart_func))
#define getboxpart (*(__currentcontext.ff.driver_getboxpart_func))
#define copybox (*(__currentcontext.ff.driver_copybox_func))


#define __currentcontext currentcontext



/* Global variables */

GraphicsContext currentcontext;

/* Initialization and graphics contexts */

void gl_setcontextvirtual(int w, int h, int bpp, int bitspp, void *v)
{
    memset(&currentcontext, 0, sizeof(GraphicsContext));
    WIDTH = w;
    HEIGHT = h;
    BYTESPERPIXEL = bpp;
    BITSPERPIXEL = bitspp;
    COLORS = 1 << bitspp;
    BYTEWIDTH = WIDTH * BYTESPERPIXEL;
    VBUF = v;
    MODETYPE = CONTEXT_VIRTUAL;
    MODEFLAGS = 0;
    __clip = 0;
}

GraphicsContext * gl_allocatecontext()
{
    return malloc(sizeof(GraphicsContext));
}

void gl_setcontext(GraphicsContext * gc)
{
    currentcontext = *gc;
}

void gl_getcontext(GraphicsContext * gc)
{
    *gc = __currentcontext;
}

void gl_freecontext(GraphicsContext * gc)
{
    if (gc->modetype == CONTEXT_VIRTUAL)
	free(gc->vbuf);
}

void gl_setcontextwidth(int w)
{
    __currentcontext.width = currentcontext.width = w;
    __currentcontext.bytewidth = currentcontext.bytewidth =
	w * BYTESPERPIXEL;
}

void gl_setcontextheight(int h)
{
    __currentcontext.height = currentcontext.height = h;
}


/* Clipping */

void gl_setclippingwindow(int x1, int y1, int x2, int y2)
{
    __clip = 1;
    __clipx1 = x1;
    __clipy1 = y1;
    __clipx2 = x2;
    __clipy2 = y2;
}

void gl_enableclipping()
{
    __clip = 1;
    __clipx1 = 0;
    __clipy1 = 0;
    __clipx2 = WIDTH - 1;
    __clipy2 = HEIGHT - 1;
}

void gl_disableclipping()
{
    __clip = 0;
}


void gl_setpixel(int x, int y, int c)
{
    return;
}

void gl_line(int x1, int y1, int x2, int y2, int c)
{
    return;
}
