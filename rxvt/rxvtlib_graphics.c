/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	graphics.c
 *----------------------------------------------------------------------*
 * $Id: graphics.c,v 1.19 1999/04/16 05:44:06 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1994      Rob Nation <nation@rocket.sanders.lockheed.com>
 *				- original version
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 * Copyright (C) 1997,1998 mj olesen <olesen@me.queensu.ca>
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

/* commands:
 * 'C' = Clear
 * 'F' = Fill
 * 'G' = Geometry
 * 'L' = Line
 * 'P' = Points
 * 'T' = Text
 * 'W' = Window
 */

/*----------------------------------------------------------------------*
 * local functions
 */
#ifdef RXVT_GRAPHICS
/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_NewWindow (rxvtlib *o, int nargs, int args[])
{E_
    int             x, y;
    unsigned int    w, h;
    Window          win;
    grwin_t        *grwin;
    Cursor          cursor;

    if (nargs != 4) {
	print_error ("NewWindow: 4 args needed, got %d\n", nargs);
	return;
    }
    x = args[0] * o->TermWin.width / GRX_SCALE + TermWin_internalBorder;
    y = args[1] * o->TermWin.height / GRX_SCALE + TermWin_internalBorder;
    w = args[2] * o->TermWin.width / GRX_SCALE;
    h = args[3] * o->TermWin.height / GRX_SCALE;

    win = XCreateSimpleWindow (o->Xdisplay, o->TermWin.vt,
			       x, y, w, h,
			       0, o->PixColors[Color_fg], o->PixColors[Color_bg]);

    cursor = XCreateFontCursor (o->Xdisplay, XC_crosshair);
    XDefineCursor (o->Xdisplay, win, cursor);
    XMapWindow (o->Xdisplay, win);
    XSelectInput (o->Xdisplay, win, ExposureMask);

    grwin = (grwin_t *) MALLOC (sizeof (grwin_t));
    grwin->win = win;
    grwin->x = x;
    grwin->y = y;
    grwin->w = w;
    grwin->h = h;
    grwin->o->screen = 0;
    grwin->prev = NULL;
    grwin->next = o->gr_root;
    if (grwin->next)
	grwin->next->prev = grwin;
    o->gr_root = grwin;
    grwin->graphics = NULL;
    o->graphics_up++;

    rxvtlib_tt_printf (o, "\033W%ld\n", (long)grwin->win);
}

/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_ClearWindow (rxvtlib *o, grwin_t * grwin)
{E_
    grcmd_t        *cmd, *next;

    for (cmd = grwin->graphics; cmd != NULL; cmd = next) {
	next = cmd->next;
	free (cmd->coords);
	if (cmd->text != NULL)
	    free (cmd->text);
	free (cmd);
    }
    grwin->graphics = NULL;
    XClearWindow (o->Xdisplay, grwin->win);
}

/*
 * arg [0] = x
 * arg [1] = y
 * arg [2] = alignment
 * arg [3] = strlen (text)
 */
/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_Text (rxvtlib *o, grwin_t * grwin, grcmd_t * data)
{E_
    int             x, y, align;

    if (data->ncoords < 4 || data->text == NULL || *(data->text) == '\0')
	return;

    x = data->coords[0] * grwin->w / GRX_SCALE;
    y = data->coords[1] * grwin->h / GRX_SCALE;
    align = data->coords[2];

    if (align & RIGHT_TEXT)
	x -= XTextWidth (o->TermWin.font, data->text, data->coords[3]);
    else if (align & HCENTER_TEXT)
	x -= (XTextWidth (o->TermWin.font, data->text, data->coords[3]) >> 1);

    if (align & TOP_TEXT)
	y += o->TermWin.font->ascent;
    else if (align & BOTTOM_TEXT)
	y -= o->TermWin.font->descent;

    if (align & VCENTER_TEXT)
	y -= o->TermWin.font->descent
	    + ((o->TermWin.font->ascent + o->TermWin.font->descent) >> 1);
    if (align & VCAPS_CENTER_TEXT)
	y += (o->TermWin.font->ascent >> 1);

    XPMClearArea (o->Xdisplay, grwin->win, x, y, Width2Pixel (data->coords[3]),
		  Height2Pixel (1), 0);
    XDrawString (o->Xdisplay, grwin->win, o->TermWin.gc, x, y,
		 data->text, data->coords[3]);
}

/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_Geometry (rxvtlib *o, grwin_t * grwin, grcmd_t * data)
{E_
    if (grwin)
	rxvtlib_tt_printf (o, "\033G%ld %d %d %u %u %d %d %ld %ld %d\n",
		   (long)grwin->win,
		   grwin->x, grwin->y, grwin->w, grwin->h,
		   o->TermWin.fwidth,
		   o->TermWin.fheight,
		   (long)GRX_SCALE * o->TermWin.fwidth / grwin->w,
		   (long)GRX_SCALE * o->TermWin.fheight / grwin->h, o->Xdepth);
    else			/* rxvt terminal window size */
	rxvtlib_tt_printf (o, "\033G0 0 0 %d %d %d %d %ld %ld %d\n",
		   o->TermWin.width - 2 * TermWin_internalBorder,
		   o->TermWin.height - 2 * TermWin_internalBorder,
		   o->TermWin.fwidth,
		   o->TermWin.fheight,
		   (long)GRX_SCALE * o->TermWin.fwidth
		   / (o->TermWin.width - 2 * TermWin_internalBorder),
		   (long)GRX_SCALE * o->TermWin.fheight
		   / (o->TermWin.height - 2 * TermWin_internalBorder), o->Xdepth);
}

/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_DestroyWindow (rxvtlib *o, grwin_t * grwin)
{E_
    grcmd_t        *cmd, *next;

    if (grwin == NULL)
	return;

    for (cmd = grwin->graphics; cmd; cmd = next) {
	next = cmd->next;
	free (cmd->coords);
	if (cmd->text != NULL)
	    free (cmd->text);
	free (cmd);
    }

    XDestroyWindow (o->Xdisplay, grwin->win);
    if (grwin->next != NULL)
	grwin->next->prev = grwin->prev;
    if (grwin->prev != NULL)
	grwin->prev->next = grwin->next;
    else
	o->gr_root = grwin->next;
    free (grwin);

    o->graphics_up--;
}

/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_Dispatch (rxvtlib *o, grwin_t * grwin, grcmd_t * data)
{E_
    int             i, n;
    union {
	XPoint          pt[NGRX_PTS / 2];
	XRectangle      rect[NGRX_PTS / 4];
    } xdata;

    if (data->color != Color_fg) {
	XGCValues       gcv;

	gcv.foreground = o->PixColors[data->color];
	XChangeGC (o->Xdisplay, o->TermWin.gc, GCForeground, &gcv);
    }
    if (grwin)
	switch (data->cmd) {
	case 'L':
	    if (data->ncoords > 3) {
		for (n = i = 0; i < data->ncoords; i += 2, n++) {
		    xdata.pt[n].x = data->coords[i] * grwin->w / GRX_SCALE;
		    xdata.pt[n].y = data->coords[i + 1] * grwin->h / GRX_SCALE;
		}
		XDrawLines (o->Xdisplay,
			    grwin->win, o->TermWin.gc, xdata.pt, n,
			    CoordModeOrigin);
	    }
	    break;

	case 'P':
	    if (data->ncoords > 3) {
		for (n = i = 0; i < data->ncoords; i += 2, n++) {
		    xdata.pt[n].x = data->coords[i] * grwin->w / GRX_SCALE;
		    xdata.pt[n].y = data->coords[i + 1] * grwin->h / GRX_SCALE;
		}
		XDrawPoints (o->Xdisplay,
			     grwin->win, o->TermWin.gc, xdata.pt, n,
			     CoordModeOrigin);
	    }
	    break;

	case 'F':
	    if (data->ncoords > 0) {
		for (n = i = 0; i < data->ncoords; i += 4, n++) {
		    xdata.rect[n].x = data->coords[i] * grwin->w / GRX_SCALE;
		    xdata.rect[n].y = data->coords[i + 1] * grwin->h
			/ GRX_SCALE;
		    xdata.rect[n].width = ((data->coords[i + 2]
					    - data->coords[i] + 1) *
					   grwin->w / GRX_SCALE);
		    xdata.rect[n].height = ((data->coords[i + 3]
					     - data->coords[i + 1] + 1) *
					    grwin->h / GRX_SCALE);
		    XPMClearArea (o->Xdisplay, grwin->win,
				  xdata.rect[n].x, xdata.rect[n].y,
				  xdata.rect[n].width, xdata.rect[n].height,
				  0);
		}
		XFillRectangles (o->Xdisplay, grwin->win, o->TermWin.gc, xdata.rect,
				 n);
	    }
	    break;
	case 'T':
	    rxvtlib_Gr_Text (o, grwin, data);
	    break;
	case 'C':
	    rxvtlib_Gr_ClearWindow (o, grwin);
	    break;
	}
    if (data->color != Color_fg) {
	XGCValues       gcv;

	gcv.foreground = o->PixColors[Color_fg];
	XChangeGC (o->Xdisplay, o->TermWin.gc, GCForeground, &gcv);
    }
}

/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_Gr_Redraw (rxvtlib *o, grwin_t * grwin)
{E_
    grcmd_t        *cmd;

    for (cmd = grwin->graphics; cmd != NULL; cmd = cmd->next)
	rxvtlib_Gr_Dispatch (o, grwin, cmd);
}
#endif				/* RXVT_GRAPHICS */

/*----------------------------------------------------------------------*
 * end of static functions
 */
/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_Gr_ButtonReport (rxvtlib *o, int but, int x, int y)
{E_
#ifdef RXVT_GRAPHICS
    grwin_t        *grwin;

    for (grwin = o->gr_root; grwin != NULL; grwin = grwin->next)
	if ((x > grwin->x)
	    && (y > grwin->y)
	    && (x < grwin->x + grwin->w)
	    && (y < grwin->y + grwin->h))
	    break;

    if (grwin == NULL)
	return;

    x = GRX_SCALE * (x - grwin->x) / grwin->w;
    y = GRX_SCALE * (y - grwin->y) / grwin->h;
    rxvtlib_tt_printf (o, "\033%c%ld;%d;%d;\n", but, (long)grwin->win, x, y);
#endif
}

/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_Gr_do_graphics (rxvtlib *o, int cmd, int nargs, int args[],
				unsigned char *text)
{E_
#ifdef RXVT_GRAPHICS
    static Window   last_id = None;
    long            win_id;
    grwin_t        *grwin;
    grcmd_t        *newcmd, *oldcmd;
    int             i;

    if (cmd == 'W') {
	rxvtlib_Gr_NewWindow (o, nargs, args);
	return;
    }
    win_id = (nargs > 0) ? (Window) args[0] : None;

    if ((cmd == 'G') && (win_id == None)) {
	rxvtlib_Gr_Geometry (o, NULL, NULL);
	return;
    }
    if ((win_id == None) && (last_id != None))
	win_id = last_id;

    if (win_id == None)
	return;

    grwin = o->gr_root;
    while ((grwin != NULL) && (grwin->win != win_id))
	grwin = grwin->next;

    if (grwin == NULL)
	return;

    if (cmd == 'G') {
	rxvtlib_Gr_Geometry (o, grwin, NULL);
	return;
    }
    nargs--;
    args++;			/* skip over window id */

/* record this new command */
    newcmd = (grcmd_t *) MALLOC (sizeof (grcmd_t));
    newcmd->ncoords = nargs;
    newcmd->coords = (int *)MALLOC ((newcmd->ncoords * sizeof (int)));

    newcmd->next = NULL;
    newcmd->cmd = cmd;
    newcmd->color = rxvtlib_scr_get_fgcolor (o);
    newcmd->text = text;

    for (i = 0; i < newcmd->ncoords; i++)
	newcmd->coords[i] = args[i];

/*
 * If newcmd == fill, and rectangle is full window, drop all prior
 * commands.
 */
    if ((newcmd->cmd == 'F') && (grwin) && (grwin->graphics)) {
	for (i = 0; i < newcmd->ncoords; i += 4) {
	    if ((newcmd->coords[i] == 0)
		&& (newcmd->coords[i + 1] == 0)
		&& (newcmd->coords[i + 2] == GRX_SCALE)
		&& (newcmd->coords[i + 3] == GRX_SCALE)) {
		/* drop previous commands */
		oldcmd = grwin->graphics;
		while (oldcmd->next != NULL) {
		    grcmd_t        *tmp = oldcmd;

		    oldcmd = oldcmd->next;
		    free (tmp);
		}
		grwin->graphics = NULL;
	    }
	}
    }
/* insert new command into command list */
    oldcmd = grwin->graphics;
    if (oldcmd == NULL)
	grwin->graphics = newcmd;
    else {
	while (oldcmd->next != NULL)
	    oldcmd = oldcmd->next;
	oldcmd->next = newcmd;
    }
    rxvtlib_Gr_Dispatch (o, grwin, newcmd);
#endif
}

/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_Gr_scroll (rxvtlib *o, int count)
{E_
#ifdef RXVT_GRAPHICS
    static short    prev_start = 0;
    grwin_t        *grwin, *next;

    if ((count == 0) && (prev_start == o->TermWin.view_start))
	return;

    prev_start = o->TermWin.view_start;

    for (grwin = o->gr_root; grwin != NULL; grwin = next) {
	next = grwin->next;
	grwin->y -= (count * o->TermWin.fheight);
	if ((grwin->y + grwin->h) < -(o->TermWin.saveLines * o->TermWin.fheight))
	    rxvtlib_Gr_DestroyWindow (o, grwin);
	else
	    XMoveWindow (o->Xdisplay, grwin->win,
			 grwin->x,
			 grwin->y + (o->TermWin.view_start * o->TermWin.fheight));
    }
#endif
}

/* EXTPROTO */
void            rxvtlib_Gr_ClearScreen (rxvtlib *o)
{E_
#ifdef RXVT_GRAPHICS
    grwin_t        *grwin, *next;

    for (grwin = o->gr_root; grwin != NULL; grwin = next) {
	next = grwin->next;
	if ((grwin->o->screen == 0) && (grwin->y + grwin->h > 0)) {
	    if (grwin->y >= 0)
		rxvtlib_Gr_DestroyWindow (o, grwin);
	    else
		XResizeWindow (o->Xdisplay, grwin->win, grwin->w, -grwin->y);
	}
    }
#endif
}

/* EXTPROTO */
void            rxvtlib_Gr_ChangeScreen (rxvtlib *o)
{E_
#ifdef RXVT_GRAPHICS
    grwin_t        *grwin, *next;

    for (grwin = o->gr_root; grwin != NULL; grwin = next) {
	next = grwin->next;
	if (grwin->y + grwin->h > 0) {
	    if (grwin->o->screen == 1) {
		XMapWindow (o->Xdisplay, grwin->win);
		grwin->o->screen = 0;
	    } else {
		XUnmapWindow (o->Xdisplay, grwin->win);
		grwin->o->screen = 1;
	    }
	}
    }
#endif
}

/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_Gr_expose (rxvtlib *o, Window win)
{E_
#ifdef RXVT_GRAPHICS
    grwin_t        *grwin;

    for (grwin = o->gr_root; grwin != NULL; grwin = grwin->next) {
	if (grwin->win == win) {
	    rxvtlib_Gr_Redraw (o, grwin);
	    break;
	}
    }
#endif
}

/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_Gr_Resize (rxvtlib *o, int w, int h)
{E_
#ifdef RXVT_GRAPHICS
    grwin_t        *grwin;

    for (grwin = o->gr_root; grwin != NULL; grwin = grwin->next) {
	if (o->TermWin.height != h) {
	    grwin->y += (o->TermWin.height - h);
	    XMoveWindow (o->Xdisplay, grwin->win,
			 grwin->x,
			 grwin->y + (o->TermWin.view_start * o->TermWin.fheight));
	}
	rxvtlib_Gr_Redraw (o, grwin);
    }
#endif
}

/* EXTPROTO */
void            rxvtlib_Gr_reset (rxvtlib *o)
{E_
#ifdef RXVT_GRAPHICS
    grwin_t        *grwin, *next;

    for (grwin = o->gr_root; grwin != NULL; grwin = next) {
	next = grwin->next;
	rxvtlib_Gr_DestroyWindow (o, grwin);
    }

    o->graphics_up = 0;
#endif
}

/* EXTPROTO */
int             rxvtlib_Gr_Displayed (rxvtlib *o)
{E_
#ifdef RXVT_GRAPHICS
    return o->graphics_up;
#else
    return 0;
#endif
}
/*----------------------- end-of-file (C source) -----------------------*/
