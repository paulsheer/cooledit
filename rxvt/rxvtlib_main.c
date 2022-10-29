/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"
#include <stringtools.h>
#ifdef UTF8_FONT
#include <app_glob.c>
#include <font.h>
#endif

/*--------------------------------*-C-*---------------------------------*
 * File:	main.c
 *----------------------------------------------------------------------*
 * $Id: main.c,v 1.66.2.7 1999/08/17 08:11:03 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey, University of Canterbury
 *				- original version
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 *				- extensive modifications
 * Copyright (C) 1995      Garrett D'Amore <garrett@netcom.com>
 * Copyright (C) 1997      mj olesen <olesen@me.QueensU.CA>
 *				- extensive modifications
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (C) 1998      Geoff Wing <gcw@pobox.com>
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
 *---------------------------------------------------------------------*/

int rxvt_get_fontwidest (rxvtlib *o, XFontStruct * f);

	/* cursor for vt window */

/* low-intensity colors *//* 0: black             (#000000) */
#ifndef NO_BRIGHTCOLOR
/* 1: red               (#CD0000) *//* 2: green             (#00CD00) *//* 3: yellow            (#CDCD00) *//* 4: blue              (#0000CD) *//* 5: magenta           (#CD00CD) *//* 6: cyan              (#00CDCD) *//* 7: white             (#FAEBD7) *//* high-intensity colors *//* 8: bright black      (#404040) */
#endif				/* NO_BRIGHTCOLOR */
/* 1/9: bright red      (#FF0000) *//* 2/10: bright green   (#00FF00) *//* 3/11: bright yellow  (#FFFF00) *//* 4/12: bright blue    (#0000FF) *//* 5/13: bright magenta (#FF00FF) *//* 6/14: bright cyan    (#00FFFF) *//* 7/15: bright white   (#FFFFFF) */
/* Color_pointer                  *//* Color_border                   */
#ifndef NO_BOLDUNDERLINE
/* Color_BD                       *//* Color_UL                       */
#endif				/* ! NO_BOLDUNDERLINE */

#ifdef MULTICHAR_SET
/* Multicharacter font names, roman fonts sized to match */

#endif				/* MULTICHAR_SET */

/*----------------------------------------------------------------------*/
/* ARGSUSED */
/* INTPROTO */
#ifdef STANDALONE
XErrorHandler   xerror_handler (const Display * display,
				const XErrorEvent * event)
{E_
    print_error ("XError: Request: %d . %d, Error: %d", event->request_code,
		 event->minor_code, event->error_code);
    return 0;
}
#endif

/* color aliases, fg/bg bright-bold */
/* INTPROTO */
void            rxvtlib_color_aliases (rxvtlib *o, int idx)
{E_
    if (o->rs[Rs_color + idx] && isdigit (*(o->rs[Rs_color + idx]))) {
	int             i = atoi (o->rs[Rs_color + idx]);

	if (i >= 8 && i <= 15) {	/* bright colors */
	    i -= 8;
#ifndef NO_BRIGHTCOLOR
	    o->rs[Rs_color + idx] = o->rs[Rs_color + minBrightCOLOR + i];
	    return;
#endif
	}
	if (i >= 0 && i <= 7)	/* normal colors */
	    o->rs[Rs_color + idx] = o->rs[Rs_color + minCOLOR + i];
    }
}

static void rxvtlib_set_colorenv (rxvtlib *o)
{
    unsigned int    i;

    o->env_fg = -1;
    o->env_bg = -1;

    for (i = Color_Black; i <= Color_White; i++) {
	if (o->PixColors[Color_fg] == o->PixColors[i]) {
	    o->env_fg = (i - Color_Black);
	    break;
	}
    }
    for (i = Color_Black; i <= Color_White; i++) {
	if (o->PixColors[Color_bg] == o->PixColors[i]) {
	    o->env_bg = (i - Color_Black);
	    break;
	}
    }
}

/*
 * find if fg/bg matches any of the normal (low-intensity) colors
 */
/* INTPROTO */
void            rxvtlib_set_colorfgbg (rxvtlib *o)
{E_
    int i;

#ifndef NO_BRIGHTCOLOR
    o->colorfgbg = DEFAULT_RSTYLE;
    for (i = minCOLOR; i <= maxCOLOR; i++) {
	if (o->PixColors[Color_fg] == o->PixColors[i]
# ifndef NO_BOLDUNDERLINE
	    && o->PixColors[Color_fg] == o->PixColors[Color_BD]
# endif				/* NO_BOLDUNDERLINE */
	    /* if we wanted boldFont to have precedence */
# if 0				/* ifndef NO_BOLDFONT */
	    && o->TermWin.boldFont == NULL
# endif				/* NO_BOLDFONT */
	    )
	    o->colorfgbg = SET_FGCOLOR (o->colorfgbg, i);
	if (o->PixColors[Color_bg] == o->PixColors[i])
	    o->colorfgbg = SET_BGCOLOR (o->colorfgbg, i);
    }
#endif
}

/* INTPROTO */
void            rxvtlib_Get_Colours (rxvtlib *o)
{E_
    int             i;

    for (i = 0; i < (o->Xdepth <= 2 ? 2 : NRS_COLORS); i++) {
	const char     *msg = "can't load color \"%s\"";
	XColor          xcol;

	if (!o->rs[Rs_color + i])
	    continue;

	if (!XParseColor (o->Xdisplay, o->Xcmap, o->rs[Rs_color + i], &xcol)
	    || !XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
	    print_error (msg, o->rs[Rs_color + i]);
#ifndef XTERM_REVERSE_VIDEO
	    if (i < 2 && (o->Options & Opt_reverseVideo)) {
		o->rs[Rs_color + i] = o->def_colorName[!i];
	    } else
#endif
		o->rs[Rs_color + i] = o->def_colorName[i];
	    if (!o->rs[Rs_color + i])
		continue;
	    if (!XParseColor (o->Xdisplay, o->Xcmap, o->rs[Rs_color + i], &xcol)
		|| !XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
		print_error (msg, o->rs[Rs_color + i]);
		switch (i) {
		case Color_fg:
		case Color_bg:
		    /* fatal: need bg/fg color */
		    print_error ("aborting");
		    o->killed = EXIT_FAILURE | DO_EXIT;
		    return;
		    /* NOTREACHED */
		    break;
#ifndef NO_CURSORCOLOR
		case Color_cursor2:
		    xcol.pixel = o->PixColors[Color_fg];
		    break;
#endif				/* ! NO_CURSORCOLOR */
		case Color_pointer:
		    xcol.pixel = o->PixColors[Color_fg];
		    break;
		default:
		    xcol.pixel = o->PixColors[Color_bg];	/* None */
		    break;
		}
	    }
	}
	o->PixColors[i] = xcol.pixel;
    }

    if (o->Xdepth <= 2 || !o->rs[Rs_color + Color_pointer])
	o->PixColors[Color_pointer] = o->PixColors[Color_fg];
    if (o->Xdepth <= 2 || !o->rs[Rs_color + Color_border])
	o->PixColors[Color_border] = o->PixColors[Color_fg];

/*
 * get scrollBar/menuBar shadow colors
 *
 * The calculations of topShadow/bottomShadow values are adapted
 * from the fvwm window manager.
 */
#ifdef KEEP_SCROLLCOLOR
    if (o->Xdepth <= 2) {		/* Monochrome */
	o->PixColors[Color_scroll] = o->PixColors[Color_fg];
	o->PixColors[Color_topShadow] = o->PixColors[Color_bg];
	o->PixColors[Color_bottomShadow] = o->PixColors[Color_bg];
    } else {
	XColor          xcol, white;

	/* bottomShadowColor */
	xcol.pixel = o->PixColors[Color_scroll];
	XQueryColor (o->Xdisplay, o->Xcmap, &xcol);

	xcol.red = ((xcol.red) / 2);
	xcol.green = ((xcol.green) / 2);
	xcol.blue = ((xcol.blue) / 2);

	if (!XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
	    print_error ("can't allocate %s", "Color_bottomShadow");
	    xcol.pixel = o->PixColors[minCOLOR];
	}
	o->PixColors[Color_bottomShadow] = xcol.pixel;

	/* topShadowColor */
# ifdef PREFER_24BIT
	white.red = white.green = white.blue = (unsigned short)~0;
	XAllocColor (o->Xdisplay, o->Xcmap, &white);
/*        XFreeColors(Xdisplay, Xcmap, &white.pixel, 1, ~0); */
# else
	white.pixel = WhitePixel (o->Xdisplay, Xscreen);
	XQueryColor (o->Xdisplay, o->Xcmap, &white);
# endif

	xcol.pixel = o->PixColors[Color_scroll];
	XQueryColor (o->Xdisplay, o->Xcmap, &xcol);

	xcol.red = max ((white.red / 5), xcol.red);
	xcol.green = max ((white.green / 5), xcol.green);
	xcol.blue = max ((white.blue / 5), xcol.blue);

	xcol.red = min (white.red, (xcol.red * 7) / 5);
	xcol.green = min (white.green, (xcol.green * 7) / 5);
	xcol.blue = min (white.blue, (xcol.blue * 7) / 5);

	if (!XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
	    print_error ("can't allocate %s", "Color_topShadow");
	    xcol.pixel = o->PixColors[Color_White];
	}
	o->PixColors[Color_topShadow] = xcol.pixel;
    }
#endif				/* KEEP_SCROLLCOLOR */

}

Cursor TermWin_cursor = 0;
Cursor Font_cursor = 0;

/* Create_Windows() - Open and map the window */
/* INTPROTO */
void            rxvtlib_Create_Windows (rxvtlib *o, int argc, const char *const *argv)
{E_
    XClassHint      classHint;
    XWMHints        wmHint;

#ifdef PREFER_24BIT
/* FIXME: free colormap */
    XSetWindowAttributes attributes;

    o->Xdepth = DefaultDepth (o->Xdisplay, Xscreen);
    o->Xcmap = DefaultColormap (o->Xdisplay, Xscreen);
    o->Xvisual = DefaultVisual (o->Xdisplay, Xscreen);
/*
 * If depth is not 24, look for a 24bit visual.
 */
    if (o->Xdepth != 24) {
	XVisualInfo     vinfo;

	if (XMatchVisualInfo (o->Xdisplay, Xscreen, 24, TrueColor, &vinfo)) {
	    o->Xdepth = 24;
	    o->Xvisual = vinfo.visual;
	    o->Xcmap = XCreateColormap (o->Xdisplay, RootWindow (o->Xdisplay, Xscreen),
				     o->Xvisual, AllocNone);
	}
    }
#endif

/* grab colors before netscape does */
    rxvtlib_Get_Colours (o);
    if (o->killed)
	return;

    rxvtlib_change_font (o, 1, NULL);
    if (o->killed)
	return;
    rxvtlib_set_colorenv (o);
    rxvtlib_szhints_set (o);

/* parent window - reverse video so we can see placement errors
 * sub-window placement & size in resize_subwindows()
 */

#if 0
#ifndef STANDALONE
    o->szHint.x = -2;
    o->szHint.y = -2;
#endif
#endif

#ifdef PREFER_24BIT
    attributes.background_pixel = o->PixColors[Color_fg];
    attributes.border_pixel = o->PixColors[Color_border];
    attributes.colormap = o->Xcmap;
#ifdef STANDALONE
    o->TermWin.parent[0] = XCreateWindow (o->Xdisplay, Xroot,
#else
    o->TermWin.parent[0] = XCreateWindow (o->Xdisplay, o->parent_window,
#endif
				       o->szHint.x, o->szHint.y,
				       o->szHint.width, o->szHint.height,
				       BORDERWIDTH,
				       o->Xdepth, InputOutput,
				       o->Xvisual,
				       CWBackPixel | CWBorderPixel |
				       CWColormap, &attributes);
#else
#ifdef STANDALONE
    o->TermWin.parent[0] = XCreateSimpleWindow (o->Xdisplay, o->parent_window,
#else
    o->TermWin.parent[0] = XCreateSimpleWindow (o->Xdisplay, Xroot,
#endif
					     o->szHint.x, o->szHint.y,
					     o->szHint.width, o->szHint.height,
					     BORDERWIDTH,
					     o->PixColors[Color_border],
					     o->PixColors[Color_fg]);
#endif
    rxvtlib_xterm_seq (o, XTerm_title, o->rs[Rs_title]);
    rxvtlib_xterm_seq (o, XTerm_iconName, o->rs[Rs_iconName]);
/* ignore warning about discarded `const' */
    classHint.res_name = (char *)o->rs[Rs_name];
    classHint.res_class = (char *)APL_CLASS;
    wmHint.input = True;
    wmHint.initial_state = (o->Options & Opt_iconic ? IconicState : NormalState);
    wmHint.window_group = o->TermWin.parent[0];
    wmHint.flags = (InputHint | StateHint | WindowGroupHint);

    XSetWMProperties (o->Xdisplay, o->TermWin.parent[0], NULL, NULL, (char **)argv,
		      argc, &o->szHint, &wmHint, &classHint);

    XSelectInput (o->Xdisplay, o->TermWin.parent[0],
		  (KeyPressMask | FocusChangeMask | PropertyChangeMask
		   | VisibilityChangeMask | StructureNotifyMask));

/* vt cursor: Black-on-White is standard, but this is more popular */
    if (!TermWin_cursor)
	TermWin_cursor = XCreateFontCursor (o->Xdisplay, XC_xterm);
    {
	XColor          fg, bg;

	fg.pixel = o->PixColors[Color_pointer];
	XQueryColor (o->Xdisplay, o->Xcmap, &fg);
	bg.pixel = o->PixColors[Color_bg];
	XQueryColor (o->Xdisplay, o->Xcmap, &bg);
	XRecolorCursor (o->Xdisplay, TermWin_cursor, &fg, &bg);
    }

/* cursor (menuBar/scrollBar): Black-on-White */
    if (!Font_cursor)
	Font_cursor = XCreateFontCursor (o->Xdisplay, XC_left_ptr);

/* the vt window */
    o->TermWin.vt = XCreateSimpleWindow (o->Xdisplay, o->TermWin.parent[0],
				      0, 0,
				      o->szHint.width, o->szHint.height,
				      0,
				      o->PixColors[Color_fg],
				      o->PixColors[Color_bg]);

    XDefineCursor (o->Xdisplay, o->TermWin.vt, TermWin_cursor);
    XSelectInput (o->Xdisplay, o->TermWin.vt,
		  (ExposureMask | ButtonPressMask | ButtonReleaseMask | PropertyChangeMask |
		   Button1MotionMask | Button3MotionMask));

/* scrollBar: size doesn't matter */
    o->scrollBar.win = XCreateSimpleWindow (o->Xdisplay, o->TermWin.parent[0],
					 0, 0,
					 1, 1,
					 0,
					 o->PixColors[Color_fg],
					 o->PixColors[Color_bg]);

    XDefineCursor (o->Xdisplay, o->scrollBar.win, Font_cursor);
    XSelectInput (o->Xdisplay, o->scrollBar.win,
		  (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		   Button1MotionMask | Button2MotionMask | Button3MotionMask));

    {				/* ONLYIF MENUBAR */
	rxvtlib_create_menuBar (o, Font_cursor);
    }

#ifdef XPM_BACKGROUND
    if (o->rs[Rs_backgroundPixmap] != NULL && !(o->Options & Opt_transparent)) {
	const char     *p = o->rs[Rs_backgroundPixmap];

	if ((p = strchr (p, ';')) != NULL) {
	    p++;
	    rxvtlib_scale_pixmap (o, p);
	}
	rxvtlib_set_bgPixmap (o, o->rs[Rs_backgroundPixmap]);
	rxvtlib_scr_touch (o);
    }
# ifdef XPM_BUFFERING
    else {
	rxvtlib_set_bgPixmap (o, "");
	rxvtlib_scr_touch (o);
    }
# endif
#endif

#ifdef UTF8_FONT
    {
	XGCValues gcvalue;
	gcvalue.foreground = o->PixColors[Color_fg];
	gcvalue.background = o->PixColors[Color_bg];
	gcvalue.graphics_exposures = 0;
	o->TermWin.gc = XCreateGC (o->Xdisplay, o->TermWin.vt, GCForeground | GCBackground | GCGraphicsExposures, &gcvalue);
    }
#else
/* graphics context for the vt window */
    {
	XGCValues       gcvalue;

	gcvalue.font = o->TermWin.font->fid;
	gcvalue.foreground = o->PixColors[Color_fg];
	gcvalue.background = o->PixColors[Color_bg];
	gcvalue.graphics_exposures = 0;
	o->TermWin.gc = XCreateGC (o->Xdisplay, o->TermWin.vt,
				GCForeground | GCBackground |
				GCFont | GCGraphicsExposures, &gcvalue);
    }
#endif
}
/* window resizing - assuming the parent window is the correct size */
/* INTPROTO */
void            rxvtlib_resize_subwindows (rxvtlib *o, int width, int height)
{E_
    int             x = 0, y = 0;
    int             old_width = o->TermWin.width, old_height = o->TermWin.height;

    o->TermWin.width = o->TermWin.ncol * o->TermWin.fwidth;
    o->TermWin.height = o->TermWin.nrow * o->TermWin.fheight;

    o->szHint.width = width;
    o->szHint.height = height;

/* size and placement */
    if (scrollbar_visible ()) {
	o->scrollBar.beg = 0;
	o->scrollBar.end = height;
#ifndef XTERM_SCROLLBAR
	/* arrows are as high as wide - leave 1 pixel gap */
# ifdef NEXT_SCROLLBAR
	o->scrollBar.end -= SB_BUTTON_TOTAL_HEIGHT + SB_PADDING;
# else
	o->scrollBar.beg += (SB_WIDTH + 1) + o->sb_shadow;
	o->scrollBar.end -= (SB_WIDTH + 1) + o->sb_shadow;
# endif
#endif
	x = (SB_WIDTH + 2 * o->sb_shadow);	/* placement of vt window */
	width -= x;
	if ((o->Options & Opt_scrollBar_right) == 0)
	    XMoveResizeWindow (o->Xdisplay, o->scrollBar.win, 0, 0, x, height);
	else {
	    XMoveResizeWindow (o->Xdisplay, o->scrollBar.win, width, 0, x, height);
	    x = 0;		/* scrollbar on right so vt window at left */
	}
    } {				/* ONLYIF MENUBAR */
	if (menubar_visible ()) {
	    y = menuBar_TotalHeight ();	/* for placement of vt window */
	    rxvtlib_Resize_menuBar (o, x, 0, width, y);
	}
    }
    XMoveResizeWindow (o->Xdisplay, o->TermWin.vt, x, y, width, height + 1);

    if (old_width)
	rxvtlib_Gr_Resize (o, old_width, old_height);

    rxvtlib_scr_clear (o);
    rxvtlib_resize_pixmap (o);
#if 0   /* makes annoying pause, so comment out */
    XSync (o->Xdisplay, False);
#endif
}

/* EXTPROTO */
void            rxvtlib_resize_all_windows (rxvtlib *o)
{E_
    rxvtlib_szhints_recalc (o);
    XSetWMNormalHints (o->Xdisplay, o->TermWin.parent[0], &o->szHint);
    rxvtlib_AddToCNQueue (o, o->szHint.width, o->szHint.height);
    XResizeWindow (o->Xdisplay, o->TermWin.parent[0], o->szHint.width, o->szHint.height);
    rxvtlib_resize_window (o, o->szHint.width, o->szHint.height);
}

/*
 * Redraw window after exposure or size change
 * width/height are those of the parent
 */
/* EXTPROTO */
void            rxvtlib_resize_window (rxvtlib *o, unsigned int width, unsigned int height)
{E_
    int             new_ncol, new_nrow;

    new_ncol = (width - o->szHint.base_width) / o->TermWin.fwidth;
    new_nrow = (height - o->szHint.base_height) / o->TermWin.fheight;
    if (o->old_height == -1 || (new_ncol != o->TermWin.ncol)
	|| (new_nrow != o->TermWin.nrow)) {
	int             curr_screen = -1;

	/* scr_reset only works on the primary screen */
	if (o->old_height != -1) {	/* this is not the first time thru */
	    rxvtlib_selection_clear (o);
	    curr_screen = rxvtlib_scr_change_screen (o, PRIMARY);
	}
	o->TermWin.ncol = new_ncol;
	o->TermWin.nrow = new_nrow;

	rxvtlib_resize_subwindows (o, width, height);
	rxvtlib_scr_reset (o, 1);

	if (curr_screen >= 0)	/* this is not the first time thru */
	    rxvtlib_scr_change_screen (o, curr_screen);
    } else if (width != o->old_width || height != o->old_height)
	rxvtlib_resize_subwindows (o, width, height);
    o->old_width = width;
    o->old_height = height;
}

/*
 * Set the width/height of the window in characters.  Units are pixels.
 * good for toggling 80/132 columns
 */
/* EXTPROTO */
void            rxvtlib_set_widthheight (rxvtlib *o, unsigned int width, unsigned int height)
{E_
    XWindowAttributes wattr;

    if (width == 0 || height == 0) {
	XGetWindowAttributes (o->Xdisplay, Xroot, &wattr);
	if (width == 0)
	    width = wattr.width - o->szHint.base_width;
	if (height == 0)
	    height = wattr.height - o->szHint.base_height;
    }

    if (width != o->TermWin.width || height != o->TermWin.height) {
	width = o->szHint.base_width + width;
	height = o->szHint.base_height + height;

	rxvtlib_AddToCNQueue (o, width, height);
	XResizeWindow (o->Xdisplay, o->TermWin.parent[0], width, height);
	rxvtlib_resize_window (o, width, height);
#ifdef USE_XIM
	rxvtlib_IMSetStatusPosition (o);
#endif
    }
}

/* INTPROTO */
void            rxvtlib_szhints_set (rxvtlib *o)
{E_
    int             x, y, flags;
    unsigned int    width, height;

    o->szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;
    o->szHint.win_gravity = NorthWestGravity;
    o->szHint.min_aspect.x = o->szHint.min_aspect.y = 1;

    flags = (o->rs[Rs_geometry] ?
	     XParseGeometry (o->rs[Rs_geometry], &x, &y, &width, &height) : 0);

    if (flags & WidthValue) {
	o->TermWin.ncol = width;
	o->szHint.flags |= USSize;
    }
    if (flags & HeightValue) {
	o->TermWin.nrow = height;
	o->szHint.flags |= USSize;
    }
    o->TermWin.width = o->TermWin.ncol * o->TermWin.fwidth;
    o->TermWin.height = o->TermWin.nrow * o->TermWin.fheight;
    rxvtlib_szhints_recalc (o);

    if (flags & XValue) {
	if (flags & XNegative) {
	    x += (DisplayWidth (o->Xdisplay, Xscreen)
		  - (o->szHint.width + TermWin_internalBorder));
	    o->szHint.win_gravity = NorthEastGravity;
	}
	o->szHint.x = x;
	o->szHint.flags |= USPosition;
    }
    if (flags & YValue) {
	if (flags & YNegative) {
	    y += (DisplayHeight (o->Xdisplay, Xscreen)
		  - (o->szHint.height + TermWin_internalBorder));
	    o->szHint.win_gravity = (o->szHint.win_gravity == NorthEastGravity ?
				  SouthEastGravity : SouthWestGravity);
	}
	o->szHint.y = y;
	o->szHint.flags |= USPosition;
    }
}

/* INTPROTO */
void            rxvtlib_szhints_recalc (rxvtlib *o)
{E_
    o->szHint.base_width = (2 * TermWin_internalBorder);
    o->szHint.base_height = (2 * TermWin_internalBorder);
    o->szHint.base_width +=
	(scrollbar_visible ()? (SB_WIDTH + 2 * o->sb_shadow) : 0);
    o->szHint.base_height += (menubar_visible ()? menuBar_TotalHeight () : 0);
    o->szHint.width_inc = o->TermWin.fwidth;
    o->szHint.height_inc = o->TermWin.fheight;
    o->szHint.min_width = o->szHint.base_width + o->szHint.width_inc;
    o->szHint.min_height = o->szHint.base_height + o->szHint.height_inc;
    o->szHint.width = o->szHint.base_width + o->TermWin.width;
    o->szHint.height = o->szHint.base_height + o->TermWin.height;
}

/* xterm sequences - title, iconName, color (exptl) */
/* INTPROTO */
void            rxvtlib_set_title (rxvtlib *o, const char *str)
{E_
#ifndef SMART_WINDOW_TITLE
    XStoreName (o->Xdisplay, o->TermWin.parent[0], str);
#else
    char           *name = NULL;

    if (!XFetchName (o->Xdisplay, o->TermWin.parent[0], &name))
	name = NULL;
    if (name == NULL || strcmp (name, str))
	XStoreName (o->Xdisplay, o->TermWin.parent[0], str);
    if (name)
	XFree (name);
#endif
}

/* INTPROTO */
void            rxvtlib_set_iconName (rxvtlib *o, const char *str)
{E_
#ifndef SMART_WINDOW_TITLE
    XSetIconName (o->Xdisplay, o->TermWin.parent[0], str);
#else
    char           *name;

    if (!XGetIconName (o->Xdisplay, o->TermWin.parent[0], &name))
	name = NULL;
    if (name == NULL || strcmp (name, str))
	XSetIconName (o->Xdisplay, o->TermWin.parent[0], str);
    if (name)
	XFree (name);
#endif
}

#ifdef XTERM_COLOR_CHANGE
/* INTPROTO */
void            rxvtlib_set_window_color (rxvtlib *o, int idx, const char *color)
{E_
    const char     *msg = "can't load color \"%s\"";
    XColor          xcol;
    int             i;

    if (color == NULL || *color == '\0')
	return;

/* handle color aliases */
    if (isdigit (*color)) {
	i = atoi (color);
	if (i >= 8 && i <= 15) {	/* bright colors */
	    i -= 8;
# ifndef NO_BRIGHTCOLOR
	    o->PixColors[idx] = o->PixColors[minBrightCOLOR + i];
	    goto Done;
# endif
	}
	if (i >= 0 && i <= 7) {	/* normal colors */
	    o->PixColors[idx] = o->PixColors[minCOLOR + i];
	    goto Done;
	}
    }
    if (!XParseColor (o->Xdisplay, o->Xcmap, color, &xcol)
	|| !XAllocColor (o->Xdisplay, o->Xcmap, &xcol)) {
	print_error (msg, color);
	return;
    }
/* XStoreColor (Xdisplay, Xcmap, XColor*); */

/*
 * FIXME: should free colors here, but no idea how to do it so instead,
 * so just keep gobbling up the colormap
 */
# if 0
    for (i = Color_Black; i <= Color_White; i++)
	if (o->PixColors[idx] == o->PixColors[i])
	    break;
    if (i > Color_White) {
	/* fprintf (stderr, "XFreeColors: PixColors [%d] = %lu\n", idx, PixColors [idx]); */
	XFreeColors (o->Xdisplay, o->Xcmap, (o->PixColors + idx), 1,
		     DisplayPlanes (o->Xdisplay, Xscreen));
    }
# endif

    o->PixColors[idx] = xcol.pixel;

/* XSetWindowAttributes attr; */
/* Cursor cursor; */
  Done:
    if (idx == Color_bg && !(o->Options & Opt_transparent))
	XSetWindowBackground (o->Xdisplay, o->TermWin.vt, o->PixColors[Color_bg]);

/* handle Color_BD, scrollbar background, etc. */

    rxvtlib_set_colorfgbg (o);
    {
	XColor          fg, bg;

	fg.pixel = o->PixColors[Color_pointer];
	XQueryColor (o->Xdisplay, o->Xcmap, &fg);
	bg.pixel = o->PixColors[Color_bg];
	XQueryColor (o->Xdisplay, o->Xcmap, &bg);
	XRecolorCursor (o->Xdisplay, TermWin_cursor, &fg, &bg);
    }
/* the only reasonable way to enforce a clean update */
    rxvtlib_scr_poweron (o);
}
#else
#endif				/* XTERM_COLOR_CHANGE */

/*
 * XTerm escape sequences: ESC ] Ps;Pt BEL
 *       0 = change iconName/title
 *       1 = change iconName
 *       2 = change title
 *      46 = change logfile (not implemented)
 *      50 = change font
 *
 * rxvt extensions:
 *      10 = menu
 *      20 = bg pixmap
 *      39 = change default fg color
 *      49 = change default bg color
 */
/* EXTPROTO */
void            rxvtlib_xterm_seq (rxvtlib *o, int op, const char *str)
{E_
    int             changed = 0;

    assert (str != NULL);
    switch (op) {
    case XTerm_name:
	rxvtlib_set_title (o, str);
	/* FALLTHROUGH */
    case XTerm_iconName:
	rxvtlib_set_iconName (o, str);
	break;
    case XTerm_title:
	rxvtlib_set_title (o, str);
	break;
    case XTerm_Menu:
	/*
	 * menubar_dispatch() violates the constness of the string,
	 * so DON'T do it here
	 */
	break;
    case XTerm_Pixmap:
	if (*str != ';') {
	    rxvtlib_scale_pixmap (o, "");	/* reset to default scaling */
	    rxvtlib_set_bgPixmap (o, str);	/* change pixmap */
	    rxvtlib_scr_touch (o);
	}
	while ((str = strchr (str, ';')) != NULL) {
	    str++;
	    changed += rxvtlib_scale_pixmap (o, str);
	}
	if (changed) {
	    rxvtlib_resize_pixmap (o);
	    rxvtlib_scr_touch (o);
	}
	break;

    case XTerm_restoreFG:
	rxvtlib_set_window_color (o, Color_fg, str);
	break;
    case XTerm_restoreBG:
	rxvtlib_set_window_color (o, Color_bg, str);
	break;
    case XTerm_logfile:
	break;
    case XTerm_font:
	rxvtlib_change_font (o, 0, str);
	break;
    }
}

/* change_font() - Switch to a new font */
/*
 * init = 1   - initialize
 *
 * fontname == FONT_UP  - switch to bigger font
 * fontname == FONT_DN  - switch to smaller font
 */
/* EXTPROTO */
void            rxvtlib_change_font (rxvtlib *o, int init, const char *fontname)
{E_
#ifndef UTF8_FONT
    const char     *msg = "can't load font \"%s\"";
#endif
    XFontStruct    *xfont = 0;
    int             idx = 0;	/* index into rs[Rs_font] */
    int             recheckfonts;
    static char    *newfont[NFONTS];
    static int      fnum;	/* logical font number */

#ifndef NO_BOLDFONT
    static XFontStruct *boldFont;
#endif

    (void) boldFont;

    if (init) {
#ifndef NO_BOLDFONT
	boldFont = NULL;
#endif
	fnum = FONT0_IDX;	/* logical font number */
    } else {
	switch (fontname[0]) {
	case '\0':
	    fnum = FONT0_IDX;
	    fontname = NULL;
	    break;

	    /* special (internal) prefix for font commands */
	case FONT_CMD:
	    idx = atoi (fontname + 1);
	    switch (fontname[1]) {
	    case '+':		/* corresponds to FONT_UP */
		fnum += (idx ? idx : 1);
		fnum = FNUM_RANGE (fnum);
		break;

	    case '-':		/* corresponds to FONT_DN */
		fnum += (idx ? idx : -1);
		fnum = FNUM_RANGE (fnum);
		break;

	    default:
		if (fontname[1] != '\0' && !isdigit (fontname[1]))
		    return;
		if (idx < 0 || idx >= (NFONTS))
		    return;
		fnum = IDX2FNUM (idx);
		break;
	    }
	    fontname = NULL;
	    break;

	default:
	    if (fontname != NULL) {
		/* search for existing fontname */
		for (idx = 0; idx < NFONTS; idx++) {
		    if (!strcmp (o->rs[Rs_font + idx], fontname)) {
			fnum = IDX2FNUM (idx);
			fontname = NULL;
			break;
		    }
		}
	    } else
		return;
	    break;
	}
	/* re-position around the normal font */
	idx = FNUM2IDX (fnum);

	if (fontname != NULL) {
	    char           *name;

	    xfont = XLoadQueryFont (o->Xdisplay, fontname);
	    if (!xfont)
		return;

	    name = MALLOC (strlen (fontname + 1) * sizeof (char));

	    if (name == NULL) {
		XFreeFont (o->Xdisplay, xfont);
		xfont = 0;
		return;
	    }
	    STRCPY (name, fontname);
	    if (newfont[idx] != NULL)
		FREE (newfont[idx]);
	    newfont[idx] = name;
	    o->rs[Rs_font + idx] = newfont[idx];
	}
    }
#ifndef UTF8_FONT
    if (o->TermWin.font) {
	XFreeFont (o->Xdisplay, o->TermWin.font);
	o->TermWin.font = 0;
    }

/* load font or substitute */
    xfont = XLoadQueryFont (o->Xdisplay, o->rs[Rs_font + idx]);
    if (!xfont) {
/* try load it with a `-*' appended */
	char *temp_font_name;
	strcpy (temp_font_name = malloc (strlen (o->rs[Rs_font + idx]) + 10), o->rs[Rs_font + idx]);
	strcat (temp_font_name, "-*");
	xfont = XLoadQueryFont (o->Xdisplay, temp_font_name);
	free (temp_font_name);
    }
    if (!xfont) {
	print_error (msg, o->rs[Rs_font + idx]);
	o->rs[Rs_font + idx] = "fixed";
	xfont = XLoadQueryFont (o->Xdisplay, o->rs[Rs_font + idx]);
	if (!xfont) {
	    print_error (msg, o->rs[Rs_font + idx]);
	    goto Abort;
	}
    }
    o->TermWin.font = xfont;
#endif

#ifndef NO_BOLDFONT
/* fail silently */
    if (init && o->rs[Rs_boldFont] != NULL)
	boldFont = XLoadQueryFont (o->Xdisplay, o->rs[Rs_boldFont]);
#endif

#ifdef MULTICHAR_SET
    if (o->TermWin.mfont) {
	XFreeFont (o->Xdisplay, o->TermWin.mfont);
	o->TermWin.mfont = 0;
    }

/* load font or substitute */
    xfont = XLoadQueryFont (o->Xdisplay, o->rs[Rs_mfont + idx]);
    if (!xfont) {
	print_error (msg, o->rs[Rs_mfont + idx]);
#ifdef ZHCN
	o->rs[Rs_mfont + idx] = "-*-16-*-gb2312*-*";
#else
	o->rs[Rs_mfont + idx] = "k14";
#endif
	xfont = XLoadQueryFont (o->Xdisplay, o->rs[Rs_mfont + idx]);
	if (!xfont) {
	    print_error (msg, o->rs[Rs_mfont + idx]);
	    goto Abort;
	}
    }
    o->TermWin.mfont = xfont;
#endif				/* MULTICHAR_SET */

/* alter existing GC */
    if (!init) {
#ifndef UTF8_FONT
	XSetFont (o->Xdisplay, o->TermWin.gc, o->TermWin.font->fid);
#endif
	rxvtlib_menubar_expose (o);
    }
/* set the sizes */
#ifdef UTF8_FONT
    CPushFont (o->fontname);
    o->TermWin.fprop = 0;
    o->TermWin.mprop = 0;
    o->TermWin.bprop = 0;
    o->TermWin.fwidth = FONT_MEAN_WIDTH;
    o->TermWin.fheight = FONT_HEIGHT;
    recheckfonts = 0;
    (void) recheckfonts;
    CPopFont ();
#else
    {
	int             fh, fw = 0;

	fw = rxvt_get_fontwidest(o, o->TermWin.font);
	fh = o->TermWin.font->ascent + o->TermWin.font->descent;

	if (fw == o->TermWin.font->min_bounds.width)
	    o->TermWin.fprop = 0;	/* Mono-spaced (fixed width) font */
	else
	    o->TermWin.fprop = 1;	/* Proportional font */

	recheckfonts = !(fw == o->TermWin.fwidth && fh == o->TermWin.fheight);
	o->TermWin.fwidth = fw;
	o->TermWin.fheight = fh;
    }

/* check that size of boldFont is okay */
#ifndef NO_BOLDFONT
    if (recheckfonts) {
	o->TermWin.boldFont = NULL;
	if (boldFont != NULL) {
	    int             fh, fw;

	    fw = rxvt_get_fontwidest(o, boldFont);
	    fh = boldFont->ascent + boldFont->descent;
	    if (fw <= o->TermWin.fwidth && fh <= o->TermWin.fheight)
		o->TermWin.boldFont = boldFont;
	    o->TermWin.bprop = !(fw == o->TermWin.fwidth /* && fh == TermWin.fheight */ );
	}
    }
#endif				/* NO_BOLDFONT */
#endif                          /* !UTF8_FONT */

#ifdef MULTICHAR_SET
    if (recheckfonts)
    /* TODO: XXX: This could be much better? */
	if (o->TermWin.mfont != NULL) {
	    int             fh, fw;

	    fw = get_fontwidest(o->TermWin.mfont);
	    fh = o->TermWin.mfont->ascent + o->TermWin.mfont->descent;
	    if (fw <= o->TermWin.fwidth && fh <= o->TermWin.fheight)
		/* WHAT TO DO!! */ ;
	    o->TermWin.mprop = !(fw == o->TermWin.fwidth /* && fh == TermWin.fheight */ );
	}
#endif

    rxvtlib_set_colorfgbg (o);

    o->TermWin.width = o->TermWin.ncol * o->TermWin.fwidth;
    o->TermWin.height = o->TermWin.nrow * o->TermWin.fheight;

    if (!init) {
	rxvtlib_resize_all_windows (o);
	rxvtlib_scr_touch (o);
    }
    return;
#ifndef UTF8_FONT
  Abort:
#endif
    print_error ("aborting");	/* fatal problem */
    o->killed = EXIT_FAILURE | DO_EXIT;
    /* NOTREACHED */
}


int rxvt_get_fontwidest (rxvtlib *o, XFontStruct * f)
{E_
    int i, cw, fw = 0;

    if (f->min_bounds.width == f->max_bounds.width)
	return f->min_bounds.width;
    if (f->per_char == NULL)
	return 0;
    for (i = f->max_char_or_byte2 - f->min_char_or_byte2; --i >= 0;) {
	cw = f->per_char[i].width;
	MAX_IT (fw, cw);
    }
    return fw;
}



/* ------------------------------------------------------------------------- */
/* INTPROTO */
void            rxvtlib_init_vars (rxvtlib *o)
{E_
    o->Options = Opt_scrollBar | Opt_scrollTtyOutput;
    o->sb_shadow = 0;
    o->TermWin.ncol = 80;
    o->TermWin.nrow = 24;
    o->TermWin.mapped = 0;
    o->want_refresh = 1;
    o->scrollBar.win = 0;
#if (MENUBAR_MAX)
    o->menuBar.win = 0;
#endif

#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
/* recognized when combined with HOTKEY */
    o->ks_bigfont = XK_greater;
    o->ks_smallfont = XK_less;
#endif
#ifndef NO_BRIGHTCOLOR
    o->colorfgbg = DEFAULT_RSTYLE;
#endif
#ifndef NO_NEW_SELECTION
    o->selection_style = NEW_SELECT;
#else
    o->selection_style = OLD_SELECT;
#endif
}

extern Display *CDisplay;

/* ------------------------------------------------------------------------- */
/* INTPROTO */
char    **rxvtlib_init_resources (rxvtlib *o, int argc, const char *const *argv)
{E_
    int             i, r_argc;
    char           *val;
    const char     *tmp;
    char           **cmd_argv, **r_argv;

/*
 * Look for -exec option.  Find => split and make cmd_argv[] of command args
 */
    for (r_argc = 0; r_argc < argc; r_argc++)
	if (!strcmp (argv[r_argc], "-e") || !strcmp (argv[r_argc], "-exec"))
	    break;
    r_argv = (char **)MALLOC (sizeof (char *) * (r_argc + 1));

    for (i = 0; i < r_argc; i++)
	r_argv[i] = (char *) argv[i];
    r_argv[i] = NULL;
    if (r_argc == argc)
	cmd_argv = NULL;
    else {
	cmd_argv = (char **)MALLOC (sizeof (char *) * (argc - r_argc));

	for (i = 0; i < argc - r_argc - 1; i++)
	    cmd_argv[i] = (char *) argv[i + r_argc + 1];
	cmd_argv[i] = NULL;
    }

/* clear all resources */
    for (i = 0; i < TOTAL_RS; i++)
	o->rs[i] = NULL;

    o->rs[Rs_name] = (char *) my_basename (argv[0]);
    if (cmd_argv != NULL && cmd_argv[0] != NULL)
	o->rs[Rs_iconName] = o->rs[Rs_title] = (char *) my_basename (cmd_argv[0]);
/*
 * Open display, get options/resources and create the window
 */
#ifdef STANDALONE
    if ((o->rs[Rs_display_name] = getenv ("DISPLAY")) == NULL)
	o->rs[Rs_display_name] = ":0";
#ifdef LOCAL_X_IS_UNIX
    if (strncmp (o->rs[Rs_display_name], ":0", 2) == 0)
	o->rs[Rs_display_name] = "unix:0";
#endif
#else
    o->rs[Rs_display_name] = getenv ("DISPLAY");
#endif

    rxvtlib_get_options (o, r_argc, r_argv);
    if (o->killed)
	return 0;

    FREE (r_argv);

#ifdef STANDALONE
    if ((o->Xdisplay = XOpenDisplay (o->rs[Rs_display_name])) == NULL) {
	print_error ("can't open display %s", o->rs[Rs_display_name]);
	o->killed = EXIT_FAILURE | DO_EXIT;
	return 0;
    }
#else
    o->Xdisplay = CDisplay;		/* FIXME: not generic */
#endif
#ifdef INEXPENSIVE_LOCAL_X_CALLS
#error
    /* it's hard to determine further if we're on a local display or not */
    o->display_is_local = o->rs[Rs_display_name][0] == ':' ? 1 : 0;
#endif

    rxvtlib_extract_resources (o, o->Xdisplay, o->rs[Rs_name]);

#if ! defined(XTERM_SCROLLBAR) && ! defined(NEXT_SCROLLBAR)
    if (!(o->Options & Opt_scrollBar_floating))
	o->sb_shadow = SHADOW;
#endif

/*
 * set any defaults not already set
 */
    if (!o->rs[Rs_title])
	o->rs[Rs_title] = o->rs[Rs_name];
    if (!o->rs[Rs_iconName])
	o->rs[Rs_iconName] = o->rs[Rs_title];
    if (!o->rs[Rs_saveLines] || (o->TermWin.saveLines = atoi (o->rs[Rs_saveLines])) < 0)
	o->TermWin.saveLines = SAVELINES;

/* no point having a scrollbar without having any scrollback! */
    if (!o->TermWin.saveLines)
	o->Options &= ~Opt_scrollBar;

#ifdef PRINTPIPE
    if (!o->rs[Rs_print_pipe])
	o->rs[Rs_print_pipe] = PRINTPIPE;
#endif
    if (!o->rs[Rs_cutchars])
	o->rs[Rs_cutchars] = CUTCHARS;
#ifndef NO_BACKSPACE_KEY
    if (!o->rs[Rs_backspace_key])
# ifdef DEFAULT_BACKSPACE
	o->key_backspace = DEFAULT_BACKSPACE;
# else
    o->key_backspace = "DEC";	/* can toggle between \033 or \177 */
# endif
    else {
	val = (char *) strdup (o->rs[Rs_backspace_key]);
	(void)Str_escaped (val);
	o->key_backspace = val;
    }
#endif
#ifndef NO_DELETE_KEY
    if (!o->rs[Rs_delete_key])
# ifdef DEFAULT_DELETE
	o->key_delete = DEFAULT_DELETE;
# else
    o->key_delete = "\033[3~";
# endif
    else {
	val = (char *) strdup (o->rs[Rs_delete_key]);
	(void)Str_escaped (val);
	o->key_delete = val;
    }
#endif

    if (o->rs[Rs_selectstyle]) {
	if (Cstrncasecmp (o->rs[Rs_selectstyle], "oldword", 7) == 0)
	    o->selection_style = OLD_WORD_SELECT;
#ifndef NO_OLD_SELECTION
	else if (Cstrncasecmp (o->rs[Rs_selectstyle], "old", 3) == 0)
	    o->selection_style = OLD_SELECT;
#endif
    }
#ifndef NO_BOLDFONT
    if (o->rs[Rs_font] == NULL && o->rs[Rs_boldFont] != NULL) {
	o->rs[Rs_font] = o->rs[Rs_boldFont];
	o->rs[Rs_boldFont] = NULL;
    }
#endif
    for (i = 0; i < NFONTS; i++) {
	if (!o->rs[Rs_font + i])
	    o->rs[Rs_font + i] = o->def_fontName[i];
#ifdef MULTICHAR_SET
	if (!o->rs[Rs_mfont + i])
	    o->rs[Rs_mfont + i] = o->def_mfontName[i];
#endif
    }
#ifndef UTF8_FONT
    o->TermWin.fontset = NULL;
#endif

#ifdef XTERM_REVERSE_VIDEO
/* this is how xterm implements reverseVideo */
    if (o->Options & Opt_reverseVideo) {
	if (!o->rs[Rs_color + Color_fg])
	    o->rs[Rs_color + Color_fg] = o->def_colorName[Color_bg];
	if (!o->rs[Rs_color + Color_bg])
	    o->rs[Rs_color + Color_bg] = o->def_colorName[Color_fg];
    }
#endif

    for (i = 0; i < NRS_COLORS; i++)
	if (!o->rs[Rs_color + i])
	    o->rs[Rs_color + i] = o->def_colorName[i];

#ifndef XTERM_REVERSE_VIDEO
/* this is how we implement reverseVideo */
    if (o->Options & Opt_reverseVideo) {
	tmp = (char *) o->rs[Rs_color + Color_fg];
	o->rs[Rs_color + Color_fg] = o->rs[Rs_color + Color_bg];
	o->rs[Rs_color + Color_bg] = (char *) tmp;
    }
#endif

/* convenient aliases for setting fg/bg to colors */
    rxvtlib_color_aliases (o, Color_fg);
    rxvtlib_color_aliases (o, Color_bg);
#ifndef NO_CURSORCOLOR
    rxvtlib_color_aliases (o, Color_cursor);
    rxvtlib_color_aliases (o, Color_cursor2);
#endif				/* NO_CURSORCOLOR */
    rxvtlib_color_aliases (o, Color_pointer);
    rxvtlib_color_aliases (o, Color_border);
#ifndef NO_BOLDUNDERLINE
    rxvtlib_color_aliases (o, Color_BD);
    rxvtlib_color_aliases (o, Color_UL);
#endif				/* NO_BOLDUNDERLINE */

    return cmd_argv;
}

/* ------------------------------------------------------------------------- */
/* INTPROTO */
static void            rxvtlib_init_display (rxvtlib *o)
{E_
    char           *val;

#ifdef DISPLAY_IS_IP
/* Fixup display_name for export over pty to any interested terminal
 * clients via "ESC[7n" (e.g. shells).  Note we use the pure IP number
 * (for the first non-loopback interface) that we get from
 * network_display().  This is more "name-resolution-portable", if you
 * will, and probably allows for faster x-client startup if your name
 * server is beyond a slow link or overloaded at client startup.  Of
 * course that only helps the shell's child processes, not us.
 *
 * Giving out the display_name also affords a potential security hole
 */
    o->rs[Rs_display_name] = (const char *)val =
	network_display (o->rs[Rs_display_name]);
    if (val == NULL)
#endif				/* DISPLAY_IS_IP */
	val = XDisplayString (o->Xdisplay);
    if (o->rs[Rs_display_name] == NULL)
	o->rs[Rs_display_name] = val;	/* use broken `:0' value */
}

void rxvt_set_input_context (rxvtlib *o, XIMStyle input_style);
XIMStyle get_input_style (void);


/* ------------------------------------------------------------------------- */
/* main() */
/* INTPROTO */
int rxvtlib_main (rxvtlib * o, const char *host, int argc, const char *const *argv, int do_sleep, char *errmsg)
{E_
    char **cmd_argv;

    rxvtlib_init_vars (o);
    cmd_argv = rxvtlib_init_resources (o, argc, (const char *const *) argv);
    if (o->killed)
	return EXIT_FAILURE;

#if (MENUBAR_MAX)
    rxvtlib_menubar_read (o, o->rs[Rs_menu]);
#endif
    rxvtlib_scrollbar_mapping (o, o->Options & Opt_scrollBar);

    rxvtlib_Create_Windows (o, argc, (const char *const *) argv);
    if (o->killed)
	return EXIT_FAILURE;

#ifdef STANDALONE
    rxvtlib_init_xlocale (o);
#endif

    rxvtlib_scr_reset (o, 0);		/* initialize screen */
    rxvtlib_Gr_reset (o);		/* reset graphics */

#ifdef STANDALONE
#ifdef DEBUG_X
    XSynchronize (o->Xdisplay, True);
    XSetErrorHandler ((XErrorHandler) abort);
#else
    XSetErrorHandler ((XErrorHandler) xerror_handler);
#endif
#endif

    if (scrollbar_visible ())
	XMapWindow (o->Xdisplay, o->scrollBar.win);
#if (MENUBAR_MAX)
    if (menubar_visible ())
	XMapWindow (o->Xdisplay, o->menuBar.win);
#endif
#ifdef TRANSPARENT
    if (o->Options & Opt_transparent) {
	XSetWindowBackgroundPixmap (o->Xdisplay, o->TermWin.parent[0],
				    ParentRelative);
	XSetWindowBackgroundPixmap (o->Xdisplay, o->TermWin.vt, ParentRelative);
	XSelectInput (o->Xdisplay, Xroot, PropertyChangeMask);
    }
#endif
    XMapWindow (o->Xdisplay, o->TermWin.vt);
    XMapWindow (o->Xdisplay, o->TermWin.parent[0]);

    rxvt_set_input_context (o, get_input_style ());

    rxvtlib_init_display (o);
    rxvtlib_init_command (o, host, cmd_argv, do_sleep, errmsg);
    if (o->killed)
	return EXIT_FAILURE;

#ifdef STANDALONE
    rxvtlib_main_loop (o);		/* main processing loop */
#endif
    if (cmd_argv)
	free (cmd_argv);
    return EXIT_SUCCESS;
}
/*----------------------- end-of-file (C source) -----------------------*/
