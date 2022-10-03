/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* triangl.c
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>

#ifndef DO_NOT_USE_VGALIB
#include <vga.h>
#endif

#include <vgagl.h>
#include "triangle.h"
#include <stdio.h>

#define SHLB 8
#define SHC 0
#ifdef WRAP
#define S_MASK 0x01ff00
#endif


#ifdef byte
#undef byte
#endif
#define byte unsigned char

#ifdef word
#undef word
#endif
#define word unsigned short

#ifdef u_32bit_t
#undef u_32bit_t
#endif

#ifdef INT_IS_16_BITS
#define u_32bit_t unsigned long
#else
#define u_32bit_t unsigned int
#endif


/* this assumes that BYTEWIDTH is not necessarily equal to bytes-per-pixel times WIDTH */

#define assignvpoffset8(x, y, vp) vp = (y) * BYTEWIDTH + (x);
#define assignvpoffset16(x, y, vp) vp = (y) * BYTEWIDTH + ((x) << 1);
#define assignvpoffset24(x, y, vp) vp = (y) * BYTEWIDTH + (x) * 3;
#define assignvpoffset32(x, y, vp) vp = (y) * BYTEWIDTH + ((x) << 2);

#define declarevp8 byte *vpbyte = (byte *) VBUF
#define declarevp16 word *vpword = (word *) VBUF
#define declarevp24 byte *vpbyte = (byte *) VBUF
#define declarevp32 u_32bit_t *vpquad = (u_32bit_t *) VBUF

#define assignvp8(x, y, vp) vpbyte = (byte *) VBUF + (y) * BYTEWIDTH + (x);
#define assignvp16(x, y, vp) vpword = (word *) ((byte *) VBUF + (y) * BYTEWIDTH) + (x);
#define assignvp24(x, y, vp) vpbyte = (byte *) VBUF + (y) * BYTEWIDTH + (x) * 3;
#define assignvp32(x, y, vp) vpquad = (u_32bit_t *) ((byte *) VBUF + (y) * BYTEWIDTH) + (x);

/* here we would like to have a single void pointer and cast it to byte, word or
u_32bit_t, but ansi does not allow casts on LHS   :(   */

#define decvp8 *(--(vpbyte)) = lookup(color)
#define incvp8 *((vpbyte)++) = lookup(color)
#define decvp16 *(--(vpword)) = lookup(color)
#define incvp16 *((vpword)++) = lookup(color)
#define decvp24 *(--(vpbyte)) = lookup(color) >> 16; \
		*(--(vpbyte)) = lookup(color) >> 8; \
		*(--(vpbyte)) = lookup(color);
#define incvp24 *((vpbyte)++) = lookup(color); \
		*((vpbyte)++) = lookup(color) >> 8; \
		*((vpbyte)++) = lookup(color) >> 16;
#define decvp32 *(--(vpquad)) = lookup(color)
#define incvp32 *((vpquad)++) = lookup(color)

#define decvpoffset8 \
		if (!offst--) \
		    vga_setpage (--pg); \
		*(vpbyte + offst) = lookup(color);
#define incvpoffset8 \
		*(vpbyte + offst) = lookup(color); \
		if (!(++offst)) \
		    vga_setpage (++pg);
#define decvpoffset16 \
		if (!offst) \
		    vga_setpage (--pg); \
		offst -= 2; \
		*(vpword + offst) = lookup(color);
#define incvpoffset16 \
		*(vpword + offst) = lookup(color); \
		offst += 2; \
		if (!offst) \
		    vga_setpage (++pg);
#define decvpoffset24 \
		if (!offst--) \
		    vga_setpage (--pg); \
		*(vpbyte + offst) = lookup(color) >> 16; \
		if (!offst--) \
		    vga_setpage (--pg); \
		*(vpbyte + offst) = lookup(color) >> 8; \
		if (!offst--) \
		    vga_setpage (--pg); \
		*(vpbyte + offst) = lookup(color);
#define incvpoffset24 \
		*(vpbyte + offst) = lookup(color); \
		if (!(++offst)) \
		    vga_setpage (++pg); \
		*(vpbyte + offst) = lookup(color) >> 8; \
		if (!(++offst)) \
		    vga_setpage (++pg); \
		*(vpbyte + offst) = lookup(color) >> 16; \
		if (!(++offst)) \
		    vga_setpage (++pg);
#define decvpoffset32 \
		if (!offst) \
		    vga_setpage (--pg); \
		offst -= 4; \
		*(vpquad + offst) = lookup(color);
#define incvpoffset32 \
		*(vpquad + offst) = lookup(color); \
		offst += 4; \
		if (!offst) \
		    vga_setpage (++pg);


static int px1, px2, py;
static long c;

#ifdef INTERP
static long c_x;
#endif

#ifdef WRAP
static long xd, xd_x, yd, yd_x;
static unsigned char *dat;
#endif

static long dx0, dy0;

#if defined(WRAP) && defined(INTERP)

/* this must only occur once */

long color_lookup[TRIANGLE_COLOR_LOOKUP_TABLE_SIZE];
static long *_color_lookup;

void gl_trisetcolorlookup (int i, long color)
{E_
    if(i < TRIANGLE_COLOR_LOOKUP_TABLE_SIZE)
	color_lookup[i] = color;
}

void (*tri_drawpoint) (int, int, int);
static void (*_tri_drawpoint) (int, int, int);

void gl_trisetdrawpoint(void (*draw_point) (int, int, int))
{E_
    tri_drawpoint = draw_point;
}

int override_8bit_non_lookup = 0;

#else

extern long color_lookup[TRIANGLE_COLOR_LOOKUP_TABLE_SIZE];
static long *_color_lookup;
extern void (*tri_drawpoint) (int, int, int);
static void (*_tri_drawpoint) (int, int, int);
extern int override_8bit_non_lookup;

#endif /* this static is just because static is faster than ordinary array (so I hear) in DLL's */


#define TRI_BPP 8
#include "trisetpixel.c"

#undef TRI_BPP
/* `9' is a pseudonam for 8lookup */
#define TRI_BPP 9
#include "trisetpixel.c"

#undef TRI_BPP
#define TRI_BPP 16
#include "trisetpixel.c"

#undef TRI_BPP
#define TRI_BPP 24
#include "trisetpixel.c"

#undef TRI_BPP
#define TRI_BPP 32
#include "trisetpixel.c"

#undef TRI_BPP


/*
   #define CONTEXT_VIRTUAL              0x0
   #define CONTEXT_PAGED                0x1
   #define CONTEXT_LINEAR               0x2
   #define CONTEXT_MODEX                0x3
   #define CONTEXT_PLANAR16             0x4
 */

static void (*linefuncs[80]) (void) =
{E_
	colhline_pos_direct8,
	colhline_neg_direct8,
	colhline_pos_paged8,
	colhline_neg_paged8,		/*2 */
	colhline_pos_direct8,
	colhline_neg_direct8,
	colhline_pos_setpixel8,
	colhline_neg_setpixel8,		/*4 */
	colhline_pos_setpixel8,
	colhline_neg_setpixel8,
	colhline_pos_setpixel8,
	colhline_neg_setpixel8,		/*6 */
	colhline_pos_setpixel8,
	colhline_neg_setpixel8,
	colhline_pos_setpixel8,
	colhline_neg_setpixel8,		/*8 */

	colhline_pos_direct16,
	colhline_neg_direct16,
	colhline_pos_paged16,
	colhline_neg_paged16,		/*2 */
	colhline_pos_direct16,
	colhline_neg_direct16,
	colhline_pos_setpixel16,
	colhline_neg_setpixel16,	/*4 */
	colhline_pos_setpixel16,
	colhline_neg_setpixel16,
	colhline_pos_setpixel16,
	colhline_neg_setpixel16,	/*6 */
	colhline_pos_setpixel16,
	colhline_neg_setpixel16,
	colhline_pos_setpixel16,
	colhline_neg_setpixel16,	/*8 */

	colhline_pos_direct24,
	colhline_neg_direct24,
	colhline_pos_paged24,
	colhline_neg_paged24,		/*2 */
	colhline_pos_direct24,
	colhline_neg_direct24,
	colhline_pos_setpixel24,
	colhline_neg_setpixel24,	/*4 */
	colhline_pos_setpixel24,
	colhline_neg_setpixel24,
	colhline_pos_setpixel24,
	colhline_neg_setpixel24,	/*6 */
	colhline_pos_setpixel24,
	colhline_neg_setpixel24,
	colhline_pos_setpixel24,
	colhline_neg_setpixel24,	/*8 */

	colhline_pos_direct32,
	colhline_neg_direct32,
	colhline_pos_paged32,
	colhline_neg_paged32,		/*2 */
	colhline_pos_direct32,
	colhline_neg_direct32,
	colhline_pos_setpixel32,
	colhline_neg_setpixel32,	/*4 */
	colhline_pos_setpixel32,
	colhline_neg_setpixel32,
	colhline_pos_setpixel32,
	colhline_neg_setpixel32,	/*6 */
	colhline_pos_setpixel32,
	colhline_neg_setpixel32,
	colhline_pos_setpixel32,
	colhline_neg_setpixel32,	/*8 */

	colhline_pos_direct8lookup,
	colhline_neg_direct8lookup,
	colhline_pos_paged8lookup,
	colhline_neg_paged8lookup,		/*2 */
	colhline_pos_direct8lookup,
	colhline_neg_direct8lookup,
	colhline_pos_setpixel8lookup,
	colhline_neg_setpixel8lookup,		/*4 */
	colhline_pos_setpixel8lookup,
	colhline_neg_setpixel8lookup,
	colhline_pos_setpixel8lookup,
	colhline_neg_setpixel8lookup,		/*6 */
	colhline_pos_setpixel8lookup,
	colhline_neg_setpixel8lookup,
	colhline_pos_setpixel8lookup,
	colhline_neg_setpixel8lookup,		/*8 */
};


#include "tri.c"
