/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	scrollbar.c
 *----------------------------------------------------------------------*
 * $Id: scrollbar.c,v 1.19.2.1 1999/07/17 09:47:26 mason Exp $
 *
 * Copyright (C) 1997,1998 mj olesen <olesen@me.QueensU.CA>
 * Copyright (C) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
 *				- N*XTstep like scrollbars
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 */
#ifndef NEXT_SCROLLBAR

#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */

	/* 12x2 bitmap */

#if (SB_WIDTH != 15)
#error Error, check scrollbar width (SB_WIDTH).It must be 15 for XTERM_SCROLLBAR
#endif

#else				/* XTERM_SCROLLBAR */

/* draw triangular button with a shadow of SHADOW (1 or 2) pixels */
/* INTPROTO */
void            rxvtlib_Draw_button (rxvtlib *o, int x, int y, int state, int dirn)
{E_
    const unsigned int sz = (SB_WIDTH), sz2 = (SB_WIDTH / 2);
    XPoint          pt[3];
    GC              top, bot;

    switch (state) {
    case +1:
	top = o->topShadowGC;
	bot = o->botShadowGC;
	break;
    case -1:
	top = o->botShadowGC;
	bot = o->topShadowGC;
	break;
    default:
	top = bot = o->scrollbarGC;
	break;
    }

/* fill triangle */
    pt[0].x = x;
    pt[1].x = x + sz - 1;
    pt[2].x = x + sz2;
    if (dirn == UP) {
	pt[0].y = pt[1].y = y + sz - 1;
	pt[2].y = y;
    } else {
	pt[0].y = pt[1].y = y;
	pt[2].y = y + sz - 1;
    }
    XFillPolygon (o->Xdisplay, o->scrollBar.win, o->scrollbarGC,
		  pt, 3, Convex, CoordModeOrigin);

/* draw base */
    XDrawLine (o->Xdisplay, o->scrollBar.win, (dirn == UP ? bot : top),
	       pt[0].x, pt[0].y, pt[1].x, pt[1].y);

/* draw shadow on left */
    pt[1].x = x + sz2 - 1;
    pt[1].y = y + (dirn == UP ? 0 : sz - 1);
    XDrawLine (o->Xdisplay, o->scrollBar.win, top,
	       pt[0].x, pt[0].y, pt[1].x, pt[1].y);

#if (SHADOW > 1)
/* doubled */
    pt[0].x++;
    if (dirn == UP) {
	pt[0].y--;
	pt[1].y++;
    } else {
	pt[0].y++;
	pt[1].y--;
    }
    XDrawLine (o->Xdisplay, o->scrollBar.win, top,
	       pt[0].x, pt[0].y, pt[1].x, pt[1].y);
#endif
/* draw shadow on right */
    pt[1].x = x + sz - 1;
/* pt[2].x = x + sz2; */
    pt[1].y = y + (dirn == UP ? sz - 1 : 0);
    pt[2].y = y + (dirn == UP ? 0 : sz - 1);
    XDrawLine (o->Xdisplay, o->scrollBar.win, bot,
	       pt[2].x, pt[2].y, pt[1].x, pt[1].y);
#if (SHADOW > 1)
/* doubled */
    pt[1].x--;
    if (dirn == UP) {
	pt[2].y++;
	pt[1].y--;
    } else {
	pt[2].y--;
	pt[1].y++;
    }
    XDrawLine (o->Xdisplay, o->scrollBar.win, bot,
	       pt[2].x, pt[2].y, pt[1].x, pt[1].y);
#endif
}
#endif				/* ! XTERM_SCROLLBAR */

#else				/* ! NEXT_SCROLLBAR */
/*
 * N*XTSTEP like scrollbar - written by Alfredo K. Kojima
 */

/* INTPROTO */
Pixmap          rxvtlib_renderPixmap (rxvtlib *o, char **data, int width, int height)
{E_
    int             x, y;
    Pixmap          d;

    d = XCreatePixmap (o->Xdisplay, o->scrollBar.win, width, height, o->Xdepth);

    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    switch (data[y][x]) {
	    case ' ':
	    case 'w':
		XDrawPoint (o->Xdisplay, d, o->whiteGC, x, y);
		break;
	    case '.':
	    case 'l':
		XDrawPoint (o->Xdisplay, d, o->grayGC, x, y);
		break;
	    case '%':
	    case 'd':
		XDrawPoint (o->Xdisplay, d, o->darkGC, x, y);
		break;
	    case '#':
	    case 'b':
	    default:
		XDrawPoint (o->Xdisplay, d, o->blackGC, x, y);
		break;
	    }
	}
    }
    return d;
}

/* INTPROTO */
void            rxvtlib_init_scrollbar_stuff (rxvtlib *o)
{E_
    XGCValues       gcvalue;
    XColor          xcol;
    Pixmap          stipple;
    unsigned long   light, dark;

    gcvalue.graphics_exposures = False;

    gcvalue.foreground =
	BlackPixelOfScreen (DefaultScreenOfDisplay (o->Xdisplay));
    o->blackGC =
	XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground | GCGraphicsExposures,
		   &gcvalue);

    gcvalue.foreground =
	WhitePixelOfScreen (DefaultScreenOfDisplay (o->Xdisplay));
    o->whiteGC =
	XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground | GCGraphicsExposures,
		   &gcvalue);

    xcol.red = 0xaeba;
    xcol.green = 0xaaaa;
    xcol.blue = 0xaeba;
    if (!XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
	print_error ("can't allocate %s", "light gray");
	xcol.pixel = o->PixColors[Color_AntiqueWhite];
    }
    light = gcvalue.foreground = xcol.pixel;
    o->grayGC =
	XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground | GCGraphicsExposures,
		   &gcvalue);

    xcol.red = 0x51aa;
    xcol.green = 0x5555;
    xcol.blue = 0x5144;
    if (!XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
	print_error ("can't allocate %s", "dark gray");
	xcol.pixel = o->PixColors[Color_Grey25];
    }
    dark = gcvalue.foreground = xcol.pixel;
    o->darkGC =
	XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground | GCGraphicsExposures,
		   &gcvalue);

    stipple = XCreateBitmapFromData (o->Xdisplay, o->scrollBar.win,
				     (char *) o->stp_bits, stp_width, stp_height);

    gcvalue.foreground = dark;
    gcvalue.background = light;
    gcvalue.fill_style = FillStippled;
    gcvalue.stipple = stipple;

/*    XSetWindowBackground(Xdisplay, scrollBar.win, PixColors[Color_Red]); */

    o->stippleGC = XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground | GCBackground
			   | GCStipple | GCFillStyle | GCGraphicsExposures,
			   &gcvalue);

    o->dimple = rxvtlib_renderPixmap (o, o->SCROLLER_DIMPLE, SCROLLER_DIMPLE_WIDTH,
			   SCROLLER_DIMPLE_HEIGHT);

    o->upArrow = rxvtlib_renderPixmap (o, o->SCROLLER_ARROW_UP, ARROW_WIDTH, ARROW_HEIGHT);
    o->downArrow = rxvtlib_renderPixmap (o, o->SCROLLER_ARROW_DOWN, ARROW_WIDTH, ARROW_HEIGHT);
    o->upArrowHi = rxvtlib_renderPixmap (o, o->HI_SCROLLER_ARROW_UP, ARROW_WIDTH, ARROW_HEIGHT);
    o->downArrowHi =
	rxvtlib_renderPixmap (o, o->HI_SCROLLER_ARROW_DOWN, ARROW_WIDTH, ARROW_HEIGHT);

    rxvtlib_scrollbar_show (o, 1);
}

/* Draw bevel & arrows */
/* INTPROTO */
void            rxvtlib_drawBevel (rxvtlib *o, Drawable d, int x, int y, int w, int h)
{E_
    XDrawLine (o->Xdisplay, d, o->whiteGC, x, y, x + w - 1, y);
    XDrawLine (o->Xdisplay, d, o->whiteGC, x, y, x, y + h - 1);

    XDrawLine (o->Xdisplay, d, o->blackGC, x + w - 1, y, x + w - 1, y + h - 1);
    XDrawLine (o->Xdisplay, d, o->blackGC, x, y + h - 1, x + w - 1, y + h - 1);

    XDrawLine (o->Xdisplay, d, o->darkGC, x + 1, y + h - 2, x + w - 2, y + h - 2);
    XDrawLine (o->Xdisplay, d, o->darkGC, x + w - 2, y + 1, x + w - 2, y + h - 2);
}

#endif				/* ! NEXT_SCROLLBAR */

/* EXTPROTO */
int             rxvtlib_scrollbar_show (rxvtlib *o, int update)
{E_
    static int      scrollbar_len;	/* length of slider */
    static int      last_top, last_bot, last_state;
#ifndef NEXT_SCROLLBAR
    static short    sb_width;		/* old (drawn) values */
    int             xsb = 0;

    if (!scrollbar_visible ())
	return 0;

    if (o->scrollbarGC == None) {
	XGCValues       gcvalue;

#ifdef XTERM_SCROLLBAR
	sb_width = SB_WIDTH - 1;
	gcvalue.stipple = XCreateBitmapFromData (o->Xdisplay, o->scrollBar.win,
						 o->sb_bits, 12, 2);
	if (!gcvalue.stipple) {
	    print_error ("can't create bitmap");
	    o->killed = EXIT_FAILURE | DO_EXIT;
	    return 0;
	}
	gcvalue.fill_style = FillOpaqueStippled;
	gcvalue.foreground = o->PixColors[Color_fg];
	gcvalue.background = o->PixColors[Color_bg];

	o->scrollbarGC = XCreateGC (o->Xdisplay, o->scrollBar.win,
				 GCForeground | GCBackground |
				 GCFillStyle | GCStipple, &gcvalue);
	gcvalue.foreground = o->PixColors[Color_border];
	o->ShadowGC = XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground, &gcvalue);
#else				/* XTERM_SCROLLBAR */
	sb_width = SB_WIDTH;

	gcvalue.foreground = o->PixColors[Color_trough];
	if (o->sb_shadow) {
	    XSetWindowBackground (o->Xdisplay, o->scrollBar.win, gcvalue.foreground);
	    XClearWindow (o->Xdisplay, o->scrollBar.win);
	}
	gcvalue.foreground = (o->Xdepth <= 2 ? o->PixColors[Color_fg]
			      : o->PixColors[Color_scroll]);
	o->scrollbarGC = XCreateGC (o->Xdisplay, o->scrollBar.win, GCForeground,
				 &gcvalue);

	gcvalue.foreground = o->PixColors[Color_topShadow];
	o->topShadowGC = XCreateGC (o->Xdisplay, o->scrollBar.win,
				 GCForeground, &gcvalue);

	gcvalue.foreground = o->PixColors[Color_bottomShadow];
	o->botShadowGC = XCreateGC (o->Xdisplay, o->scrollBar.win,
				 GCForeground, &gcvalue);
#endif				/* XTERM_SCROLLBAR */
    }
    if (update) {
	int             top = (o->TermWin.nscrolled - o->TermWin.view_start);
	int             bot = top + (o->TermWin.nrow - 1);
	int             len =

	    max ((o->TermWin.nscrolled + (o->TermWin.nrow - 1)), 1);
	int             adj = ((bot - top) * scrollbar_size()) % len;

	o->scrollBar.top = (o->scrollBar.beg + (top * scrollbar_size ()) / len);
        scrollbar_len = (((bot - top) * scrollbar_size()) / len +
			 SCROLL_MINHEIGHT + ((adj > 0) ? 1 : 0));
	o->scrollBar.bot = (o->scrollBar.top + scrollbar_len);
	/* no change */
	if ((o->scrollBar.top == last_top) && (o->scrollBar.bot == last_bot)
	    && ((o->scrollBar.state == last_state) || (!scrollbar_isUpDn())))
	    return 0;
    }
/* instead of XClearWindow (Xdisplay, scrollBar.win); */
#ifdef XTERM_SCROLLBAR
    xsb = (o->Options & Opt_scrollBar_right) ? 1 : 0;
#endif
    if (last_top < o->scrollBar.top)
	XClearArea (o->Xdisplay, o->scrollBar.win,
		    o->sb_shadow + xsb, last_top,
		    sb_width, (o->scrollBar.top - last_top), False);

    if (o->scrollBar.bot < last_bot)
	XClearArea (o->Xdisplay, o->scrollBar.win,
		    o->sb_shadow + xsb, o->scrollBar.bot,
		    sb_width, (last_bot - o->scrollBar.bot), False);

    last_top = o->scrollBar.top;
    last_bot = o->scrollBar.bot;

/* scrollbar slider */
#ifdef XTERM_SCROLLBAR
    XFillRectangle (o->Xdisplay, o->scrollBar.win, o->scrollbarGC,
		    xsb + 1, o->scrollBar.top,
		    sb_width - 2, scrollbar_len);

    XDrawLine (o->Xdisplay, o->scrollBar.win, o->ShadowGC,
	      xsb ? 0 : o->sb_width, o->scrollBar.beg, xsb ? 0 : sb_width,
	      o->scrollBar.end);

#else
#ifdef SB_BORDER
    {
	int             xofs;

	if (o->Options & Opt_scrollBar_right)
	    xofs = 0;
	else
	    xofs = (o->sb_shadow) ? SB_WIDTH : SB_WIDTH - 1;

	XDrawLine (o->Xdisplay, o->scrollBar.win, o->botShadowGC,
		   xofs, 0, xofs, o->scrollBar.end + SB_WIDTH);
    }
#endif
    XFillRectangle (o->Xdisplay, o->scrollBar.win, o->scrollbarGC,
		    o->sb_shadow, o->scrollBar.top,
		   sb_width, scrollbar_len);

    if (o->sb_shadow)
	/* trough shadow */
	rxvtlib_Draw_Shadow (o, o->scrollBar.win,
		     o->botShadowGC, o->topShadowGC,
		     0, 0,
		     (sb_width + 2 * o->sb_shadow),
		     (o->scrollBar.end + (sb_width + 1) + o->sb_shadow));
/* shadow for scrollbar slider */
    rxvtlib_Draw_Shadow (o, o->scrollBar.win,
		 o->topShadowGC, o->botShadowGC,
		 o->sb_shadow, o->scrollBar.top, sb_width,
		scrollbar_len);

/*
 * Redraw scrollbar arrows
 */
    rxvtlib_Draw_button (o, o->sb_shadow, o->sb_shadow, (scrollbar_isUp ()? -1 : +1), UP);
    rxvtlib_Draw_button (o, o->sb_shadow, (o->scrollBar.end + 1),
		 (scrollbar_isDn ()? -1 : +1), DN);
#endif				/* XTERM_SCROLLBAR */
    last_top = o->scrollBar.top;
    last_bot = o->scrollBar.bot;
    last_state = o->scrollBar.state;

#else				/* NEXT_SCROLLBAR */
    Pixmap          buffer;
    int             height = o->scrollBar.end + SB_BUTTON_TOTAL_HEIGHT + SB_PADDING;

    if (o->blackGC == NULL)
	rxvtlib_init_scrollbar_stuff (o);
    if (o->killed)
	return 0;

    if (update) {
	int             top = (o->TermWin.nscrolled - o->TermWin.view_start);
	int             bot = top + (o->TermWin.nrow - 1);
	int             len =

	    max ((o->TermWin.nscrolled + (o->TermWin.nrow - 1)), 1);
	int             adj = ((bot - top) * scrollbar_size()) % len;

	o->scrollBar.top = (o->scrollBar.beg + (top * scrollbar_size ()) / len);
        scrollbar_len = (((bot - top) * scrollbar_size()) / len +
			 SCROLL_MINHEIGHT + ((adj > 0) ? 1 : 0));
	o->scrollBar.bot = (o->scrollBar.top + scrollbar_len);
	/* no change */
	if ((o->scrollBar.top == last_top) && (o->scrollBar.bot == last_bot)
	    && ((o->scrollBar.state == last_state) || (!scrollbar_isUpDn())))
	    return 0;
    }
/* create double buffer */
    buffer =
	XCreatePixmap (o->Xdisplay, o->scrollBar.win, SB_WIDTH + 1, height, o->Xdepth);

    last_top = o->scrollBar.top;
    last_bot = o->scrollBar.bot;
    last_state = o->scrollBar.state;

/* draw the background */
    XFillRectangle (o->Xdisplay, buffer, o->grayGC, 0, 0, SB_WIDTH + 1, height);
    XDrawRectangle(o->Xdisplay, buffer, o->blackGC, 0, -SB_BORDER_WIDTH,
		   SB_WIDTH, height + SB_BORDER_WIDTH);

    if (o->TermWin.nscrolled > 0) {
	XFillRectangle(o->Xdisplay, buffer, o->stippleGC,
		       SB_LEFT_PADDING, SB_PADDING,
		       SB_BUTTON_WIDTH,
		       height - SB_BUTTON_TOTAL_HEIGHT - SB_PADDING);
	XFillRectangle(o->Xdisplay, buffer, o->grayGC,
		       SB_LEFT_PADDING, o->scrollBar.top + SB_PADDING,
		       SB_BUTTON_WIDTH, scrollbar_len);
	rxvtlib_drawBevel(o, buffer, SB_BUTTON_BEVEL_X, o->scrollBar.top + SB_PADDING,
		  SB_BUTTON_WIDTH, scrollbar_len);
	rxvtlib_drawBevel(o, buffer, SB_BUTTON_BEVEL_X, height - SB_BUTTON_BOTH_HEIGHT,
		  SB_BUTTON_WIDTH, SB_BUTTON_HEIGHT);
	rxvtlib_drawBevel(o, buffer, SB_BUTTON_BEVEL_X, height - SB_BUTTON_SINGLE_HEIGHT,
		  SB_BUTTON_WIDTH, SB_BUTTON_HEIGHT);

	XCopyArea (o->Xdisplay, o->dimple, buffer, o->whiteGC, 0, 0,
		   SCROLLER_DIMPLE_WIDTH, SCROLLER_DIMPLE_HEIGHT,
		  (SB_WIDTH - SCROLLER_DIMPLE_WIDTH) / 2,
		  o->scrollBar.top + SB_BEVEL_WIDTH_UPPER_LEFT +
		  (scrollbar_len - SCROLLER_DIMPLE_HEIGHT) / 2);

	if (scrollbar_isUp ())
	    XCopyArea(o->Xdisplay, o->upArrowHi, buffer, o->whiteGC, 0, 0,
		      ARROW_WIDTH, ARROW_HEIGHT,
		      SB_BUTTON_FACE_X,
		      height - (SB_BUTTON_BOTH_HEIGHT - SB_BEVEL_WIDTH_UPPER_LEFT));
	else
	    XCopyArea(o->Xdisplay, o->upArrow, buffer, o->whiteGC, 0, 0,
		      ARROW_WIDTH, ARROW_HEIGHT,
		      SB_BUTTON_FACE_X,
		      height - (SB_BUTTON_BOTH_HEIGHT - SB_BEVEL_WIDTH_UPPER_LEFT));

	if (scrollbar_isDn ())
	    XCopyArea(o->Xdisplay, o->downArrowHi, buffer, o->whiteGC, 0, 0,
		      ARROW_WIDTH, ARROW_HEIGHT,
		      SB_BUTTON_FACE_X,
		      height - (SB_BUTTON_SINGLE_HEIGHT - SB_BEVEL_WIDTH_UPPER_LEFT));
	else
	    XCopyArea(o->Xdisplay, o->downArrow, buffer, o->whiteGC, 0, 0,
		      ARROW_WIDTH, ARROW_HEIGHT,
		      SB_BUTTON_FACE_X,
		      height - (SB_BUTTON_SINGLE_HEIGHT - SB_BEVEL_WIDTH_UPPER_LEFT));
    } else {
	XFillRectangle(o->Xdisplay, buffer, o->stippleGC,
		       SB_LEFT_PADDING, SB_PADDING,
		       SB_BUTTON_WIDTH, height - SB_MARGIN_SPACE);
    }

    if (o->Options & Opt_scrollBar_right)
	XCopyArea (o->Xdisplay, buffer, o->scrollBar.win, o->grayGC, 0, 0,
		  SB_WIDTH + SB_BORDER_WIDTH, height, 0, 0);
    else
	XCopyArea (o->Xdisplay, buffer, o->scrollBar.win, o->grayGC, 0, 0,
		  SB_WIDTH + SB_BORDER_WIDTH, height, -SB_BORDER_WIDTH, 0);

    XFreePixmap (o->Xdisplay, buffer);
#endif				/* ! NEXT_SCROLLBAR */
    return 1;
}

/* EXTPROTO */
int             rxvtlib_scrollbar_mapping (rxvtlib *o, int map)
{E_
    int             change = 0;

    if (map && !scrollbar_visible ()) {
	o->scrollBar.state = 1;
	if (o->scrollBar.win == 0)
	    return 0;
	XMapWindow (o->Xdisplay, o->scrollBar.win);
	change = 1;
    } else if (!map && scrollbar_visible ()) {
	o->scrollBar.state = 0;
	XUnmapWindow (o->Xdisplay, o->scrollBar.win);
	change = 1;
    }
    return change;
}

/* EXTPROTO */
void            rxvtlib_map_scrollBar (rxvtlib *o, int map)
{E_
    if (rxvtlib_scrollbar_mapping (o, map)) {
	rxvtlib_resize_all_windows (o);
	rxvtlib_scr_touch (o);
    }
}

/*----------------------- end-of-file (C source) -----------------------*/
