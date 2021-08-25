/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	xpm.c
 *----------------------------------------------------------------------*
 * $Id: xpm.c,v 1.28 1999/03/11 14:09:21 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1997      Carsten Haitzler <raster@zip.com.au>
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

/* the originally loaded pixmap and its scaling */

/*
 * These GEOM strings indicate absolute size/position:
 * @ `WxH+X+Y'
 * @ `WxH+X'    -> Y = X
 * @ `WxH'      -> Y = X = 50
 * @ `W+X+Y'    -> H = W
 * @ `W+X'      -> H = W, Y = X
 * @ `W'        -> H = W, X = Y = 50
 * @ `0xH'      -> H *= H/100, X = Y = 50 (W unchanged)
 * @ `Wx0'      -> W *= W/100, X = Y = 50 (H unchanged)
 * @ `=+X+Y'    -> (H, W unchanged)
 * @ `=+X'      -> Y = X (H, W unchanged)
 *
 * These GEOM strings adjust position relative to current position:
 * @ `+X+Y'
 * @ `+X'       -> Y = X
 *
 * And this GEOM string is for querying current scale/position:
 * @ `?'
 */
/* EXTPROTO */
int             rxvtlib_scale_pixmap (rxvtlib *o, const char *geom)
{E_
#ifdef XPM_BACKGROUND
    int             w = 0, h = 0, x = 0, y = 0;
    int             n, flags, changed = 0;
    char           *p;
    static char     str[] = "[1000x1000+1000+1000]";

    if (geom == NULL)
	return 0;
    if (!strcmp (geom, "?")) {
	sprintf (str, "[%dx%d+%d+%d]",	/* can't presume snprintf? */
		 (o->bgPixmap.w < 9999 ? o->bgPixmap.w : 9999),
		 (o->bgPixmap.h < 9999 ? o->bgPixmap.h : 9999),
		 (o->bgPixmap.x < 9999 ? o->bgPixmap.x : 9999),
		 (o->bgPixmap.y < 9999 ? o->bgPixmap.y : 9999));
	rxvtlib_xterm_seq (o, XTerm_title, str);
	return 0;
    }

    if ((p = strchr (geom, ';')) == NULL)
	p = strchr (geom, '\0');
    n = (p - geom);
    if (n >= sizeof (str) - 1)
	return 0;
    STRNCPY (str, geom, n);
    str[n] = '\0';

    flags =
	XParseGeometry (str, &x, &y, (unsigned int *)&w, (unsigned int *)&h);
    if (!flags) {
	flags |= WidthValue;
	w = 0;
    }				/* default is tile */
    if (flags & WidthValue) {
	if (!(flags & XValue))
	    x = 50;

	if (!(flags & HeightValue))
	    h = w;

	if (w && !h) {
	    w = o->bgPixmap.w * ((float)w / 100);
	    h = o->bgPixmap.h;
	} else if (h && !w) {
	    w = o->bgPixmap.w;
	    h = o->bgPixmap.h * ((float)h / 100);
	}
	if (w > 1000)
	    w = 1000;
	if (h > 1000)
	    h = 1000;

	if (o->bgPixmap.w != w) {
	    o->bgPixmap.w = w;
	    changed++;
	}
	if (o->bgPixmap.h != h) {
	    o->bgPixmap.h = h;
	    changed++;
	}
    }
    if (!(flags & YValue)) {
	if (flags & XNegative)
	    flags |= YNegative;
	y = x;
    }

    if (!(flags & WidthValue) && geom[0] != '=') {
	x += o->bgPixmap.x;
	y += o->bgPixmap.y;
    } else {
	if (flags & XNegative)
	    x += 100;
	if (flags & YNegative)
	    y += 100;
    }
    MIN_IT (x, 100);
    MIN_IT (y, 100);
    MAX_IT (x, 0);
    MAX_IT (y, 0);
    if (o->bgPixmap.x != x) {
	o->bgPixmap.x = x;
	changed++;
    }
    if (o->bgPixmap.y != y) {
	o->bgPixmap.y = y;
	changed++;
    }
    return changed;
#else
    return 0;
#endif				/* XPM_BACKGROUND */
}

/* EXTPROTO */
void            rxvtlib_resize_pixmap (rxvtlib *o)
{E_
#ifdef XPM_BACKGROUND
    XGCValues       gcvalue;
    GC              gc;
    int             tiled = 0;
    unsigned int    width = TermWin_TotalWidth ();
    unsigned int    height = TermWin_TotalHeight ();

    if (o->TermWin.pixmap)
	XFreePixmap (o->Xdisplay, o->TermWin.pixmap);

# ifndef XPM_BUFFERING
    if (o->bgPixmap.pixmap == None) {	/* So be it: I'm not using pixmaps */
	o->TermWin.pixmap = None;
	if (!(o->Options & Opt_transparent))
	    XSetWindowBackground (o->Xdisplay, o->TermWin.vt, o->PixColors[Color_bg]);
	return;
    }
# endif

    gcvalue.foreground = o->PixColors[Color_bg];
    gc = XCreateGC (o->Xdisplay, o->TermWin.vt, GCForeground, &gcvalue);

    if (o->bgPixmap.pixmap) {	/* we have a specified pixmap */
	int             w = o->bgPixmap.w, h = o->bgPixmap.h,

	    x = o->bgPixmap.x, y = o->bgPixmap.y;

	/*
	 * don't zoom pixmap too much nor expand really small pixmaps
	 */
	if (w > 1000 || h > 1000)
	    w = 1;
	else if (width > (10 * o->xpmAttr.width)
		 || height > (10 * o->xpmAttr.height))
	    w = 0;		/* tile */

	if (w == 0) {
	    /* basic X tiling - let the X server do it */
	    o->TermWin.pixmap = XCreatePixmap (o->Xdisplay, o->TermWin.vt,
					    o->xpmAttr.width, o->xpmAttr.height,
					    o->Xdepth);
	    XCopyArea (o->Xdisplay, o->bgPixmap.pixmap, o->TermWin.pixmap,
		       gc, 0, 0, o->xpmAttr.width, o->xpmAttr.height, 0, 0);
	    tiled = 1;
	} else {
	    float           p, incr;
	    Pixmap          tmp;

	    o->TermWin.pixmap = XCreatePixmap (o->Xdisplay, o->TermWin.vt,
					    width, height, o->Xdepth);
	    /*
	     * horizontal scaling
	     */
	    incr = (float)o->xpmAttr.width;
	    p = 0;

	    if (w == 1) {	/* display one image, no horizontal scaling */
		incr = width;
		if (o->xpmAttr.width <= width) {
		    w = o->xpmAttr.width;
		    x = x * (width - w) / 100;	/* beware! order */
		    w += x;
		} else {
		    x = 0;
		    w = width;
		}
	    } else if (w < 10) {	/* fit W images across screen */
		incr *= w;
		x = 0;
		w = width;
	    } else {
		incr *= 100.0 / w;
		if (w < 100) {	/* contract */
		    w = (w * width) / 100;
		    if (x >= 0) {	/* position */
			float           pos;

			pos = (float)x / 100 * width - (w / 2);

			x = (width - w);
			if (pos <= 0)
			    x = 0;
			else if (pos < x)
			    x = pos;
		    } else {
			x = x * (width - w) / 100;	/* beware! order */
		    }
		    w += x;
		} else {	/* expand */
		    if (x > 0) {	/* position */
			float           pos;

			pos = (float)x / 100 * o->xpmAttr.width - (incr / 2);
			p = o->xpmAttr.width - (incr);
			if (pos <= 0)
			    p = 0;
			else if (pos < p)
			    p = pos;
		    }
		    x = 0;
		    w = width;
		}
	    }
	    incr /= width;

	    tmp = XCreatePixmap (o->Xdisplay, o->TermWin.vt,
				 width, o->xpmAttr.height, o->Xdepth);
	    XFillRectangle (o->Xdisplay, tmp, gc, 0, 0, width, o->xpmAttr.height);

	    for ( /*nil */ ; x < w; x++, p += incr) {
		if (p >= o->xpmAttr.width)
		    p = 0;
		if (x < 0)
		    continue;
		/* copy one column from the original pixmap to the tmp pixmap */
		XCopyArea (o->Xdisplay, o->bgPixmap.pixmap, tmp, gc,
			   (int)p, 0, 1, o->xpmAttr.height, x, 0);
	    }

	    /*
	     * vertical scaling
	     */
	    incr = (float)o->xpmAttr.height;
	    p = 0;

	    if (h == 1) {	/* display one image, no vertical scaling */
		incr = height;
		if (o->xpmAttr.height <= height) {
		    h = o->xpmAttr.height;
		    y = y * (height - h) / 100;	/* beware! order */
		    h += y;
		} else {
		    y = 0;
		    h = height;
		}
	    } else if (h < 10) {	/* fit H images across screen */
		incr *= h;
		y = 0;
		h = height;
	    } else {
		incr *= 100.0 / h;
		if (h < 100) {	/* contract */
		    h = (h * height) / 100;
		    if (y >= 0) {	/* position */
			float           pos;

			pos = (float)y / 100 * height - (h / 2);

			y = (height - h);
			if (pos < 0.0f)
			    y = 0;
			else if (pos < y)
			    y = pos;
		    } else {
			y = y * (height - h) / 100;	/* beware! order */
		    }
		    h += y;
		} else {	/* expand */
		    if (y > 0) {	/* position */
			float           pos;

			pos = (float)y / 100 * o->xpmAttr.height - (incr / 2);
			p = o->xpmAttr.height - (incr);
			if (pos < 0)
			    p = 0;
			else if (pos < p)
			    p = pos;
		    }
		    y = 0;
		    h = height;
		}
	    }
	    incr /= height;

	    if (y > 0)
		XFillRectangle (o->Xdisplay, o->TermWin.pixmap, gc, 0, 0, width, y);
	    if (h < height)
		XFillRectangle (o->Xdisplay, o->TermWin.pixmap, gc, 0, h,
				width, height - h + 1);
	    for ( /*nil */ ; y < h; y++, p += incr) {
		if (p >= o->xpmAttr.height)
		    p = 0;
		if (y < 0)
		    continue;
		/* copy one row from the tmp pixmap to the main pixmap */
		XCopyArea (o->Xdisplay, tmp, o->TermWin.pixmap, gc,
			   0, (int)p, width, 1, 0, y);
	    }
	    XFreePixmap (o->Xdisplay, tmp);
	}
    }
# ifdef XPM_BUFFERING
    if (o->TermWin.buf_pixmap)
	XFreePixmap (o->Xdisplay, o->TermWin.buf_pixmap);
    o->TermWin.buf_pixmap = XCreatePixmap (o->Xdisplay, o->TermWin.vt,
					width, height, o->Xdepth);
    if (!o->bgPixmap.pixmap) {	/* we don't have a specified pixmap */
	o->TermWin.pixmap = XCreatePixmap (o->Xdisplay, o->TermWin.vt,
					width, height, o->Xdepth);
	XFillRectangle (o->Xdisplay, o->TermWin.pixmap, gc, 0, 0, width, height);
    }
    if (tiled == 0) {
	XCopyArea (o->Xdisplay, o->TermWin.pixmap, o->TermWin.buf_pixmap, gc,
		   0, 0, width, height, 0, 0);
    } else {
	int             i, j, w, h;

	for (j = 0; j < height; j += o->xpmAttr.height) {
	    h = (j + o->xpmAttr.height > height) ? (height - j) : o->xpmAttr.height;
	    for (i = 0; i < width; i += o->xpmAttr.width) {
		w = (i + o->xpmAttr.width > width) ? (width - i) : o->xpmAttr.width;
		XCopyArea (o->Xdisplay, o->TermWin.pixmap, o->TermWin.buf_pixmap, gc,
			   0, 0, w, h, i, j);
	    }
	}
	XFreePixmap (o->Xdisplay, o->TermWin.pixmap);
	o->TermWin.pixmap = XCreatePixmap (o->Xdisplay, o->TermWin.vt,
					width, height, o->Xdepth);
	XCopyArea (o->Xdisplay, o->TermWin.buf_pixmap, o->TermWin.pixmap, gc,
		   0, 0, width, height, 0, 0);
    }
    XSetWindowBackgroundPixmap (o->Xdisplay, o->TermWin.vt, o->TermWin.buf_pixmap);
# else				/* XPM_BUFFERING */
    XSetWindowBackgroundPixmap (o->Xdisplay, o->TermWin.vt, o->TermWin.pixmap);
# endif				/* XPM_BUFFERING */
    XFreeGC (o->Xdisplay, gc);

    XClearWindow (o->Xdisplay, o->TermWin.vt);

    XSync (o->Xdisplay, False);
#endif				/* XPM_BACKGROUND */
}

/* EXTPROTO */
Pixmap          rxvtlib_set_bgPixmap (rxvtlib *o, const char *file)
{E_
#ifdef XPM_BACKGROUND
    char           *f;

    assert (file != NULL);

    if (o->bgPixmap.pixmap != None) {
	XFreePixmap (o->Xdisplay, o->bgPixmap.pixmap);
	o->bgPixmap.pixmap = None;
    }
    XSetWindowBackground (o->Xdisplay, o->TermWin.vt, o->PixColors[Color_bg]);

    if (*file != '\0') {
/*      XWindowAttributes attr; */

	/*
	 * we already have the required attributes
	 */
/*      XGetWindowAttributes(Xdisplay, TermWin.vt, &attr); */

	o->xpmAttr.closeness = 30000;
	o->xpmAttr.colormap = o->Xcmap;
	o->xpmAttr.visual = o->Xvisual;
	o->xpmAttr.depth = o->Xdepth;
	o->xpmAttr.valuemask = (XpmCloseness | XpmColormap | XpmVisual |
			     XpmDepth | XpmSize | XpmReturnPixels);

	/* search environment variables here too */
	if ((f = (char *)rxvtlib_File_find (o, file, ".xpm")) == NULL
	    || XpmReadFileToPixmap (o->Xdisplay, Xroot, f,
				    &o->bgPixmap.pixmap, NULL, &o->xpmAttr)) {
	    char           *p;

	    /* semi-colon delimited */
	    if ((p = strchr (file, ';')) == NULL)
		p = strchr (file, '\0');

	    print_error ("couldn't load XPM file \"%.*s\"", (p - file), file);
	}
	FREE (f);
    }
    rxvtlib_resize_pixmap (o);
    return o->bgPixmap.pixmap;
#else
    return 0;
#endif				/* XPM_BACKGROUND */
}
