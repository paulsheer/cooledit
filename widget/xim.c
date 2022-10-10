/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* xim.c - XIM handlers, for multiple locale input methods.
   Copyright (C) 1996-2022 Paul Sheer
 */


/* #define DEBUG */
/* #define DEBUG_ENTRY */

#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"

#include "coollocal.h"

#include "font.h"


/* #define XIM_DEBUG */


/* A lot of this stuff is hatched from rxvt-2.6.1: */

/*--------------------------------*-C-*---------------------------------*
 * File:	command.c
 *----------------------------------------------------------------------*
 * $Id: command.c,v 1.85.2.23 1999/08/12 16:32:39 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 * 				- extensive modifications
 * Copyright (C) 1995      Garrett D'Amore <garrett@netcom.com>
 *				- vt100 printing
 * Copyright (C) 1995      Steven Hirsch <hirsch@emba.uvm.edu>
 *				- X11 mouse report mode and support for
 *				  DEC "private mode" save/restore functions.
 * Copyright (C) 1995      Jakub Jelinek <jj@gnu.ai.mit.edu>
 *				- key-related changes to handle Shift+function
 *				  keys properly.
 * Copyright (C) 1997      MJ Olesen <olesen@me.queensu.ca>
 *				- extensive modifications
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 *				- modification and cleanups for Solaris 2.x
 *				  and Linux 1.2.x
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (C) 1998      Geoff Wing <gcw@pobox.com>
 * Copyright (C) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
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

#ifndef HAVE_XPOINTER
typedef char   *XPointer;
#endif

#ifndef USE_XIM
int option_use_xim = 0;
#endif

#ifdef USE_XIM

#define MAX_KBUF        128

static void xim_print_error (char *s,...)
{E_
    char k[1024];
    va_list ap;
    va_start (ap, s);
    vsprintf (k, s, ap);
    fprintf (stderr, "%s: %s\n", CAppName, k);
    va_end (ap);
}

int option_use_xim = 1;
static int x_server_xim_reported_enabled = 0;
static XPoint hold_spot;

static void IMInstantiateCallback (Display * display, XPointer client_data, XPointer call_data);

void init_xlocale (void)
{E_
#ifdef LC_CTYPE
    char *xim_locale = NULL;
    xim_locale = setlocale (LC_CTYPE, 0);
    CPushFont ("editor", 0);
    if (!xim_locale) {
	xim_print_error ("Setting locale failed.");
    } else if (!current_font->f.font_set) {
	xim_print_error ("Font set not loaded - cannot create input method.");
    } else {
	if (option_use_xim) {
            if (XRegisterIMInstantiateCallback (CDisplay, NULL, NULL, NULL, IMInstantiateCallback, NULL) == False)
	        xim_print_error ("XRegisterIMInstantiateCallback returned error.");
        }
    }
    CPopFont ();
#else
    option_use_xim = 0;
#endif
}

void shutdown_xlocale(void)
{E_
    if (option_use_xim) {
        if (CIM) {
            XCloseIM (CIM);
            CIM = 0;
        }
    }
}

static void setPosition (Window win, XPoint * pos)
{E_
#if 0
    XWindowAttributes xwa;
    memset(pos, '\0', sizeof (XPoint));
    XGetWindowAttributes (CDisplay, win, &xwa);
#endif
    pos->x = 20;
    pos->y = 20;
}

static XIC get_window_ic (Window win, CWidget **child_return, CWidget **mainw_return)
{E_
    CWidget *child, *mainw;
    if (!win)
	return 0;
    if (!x_server_xim_reported_enabled)
        return 0;
    if (!(child = CWidgetOfWindow (win)))
        return 0;
    if (child->mainid)
        mainw = CWidgetOfWindow (child->mainid);
    else
        mainw = child;
    if (!mainw)
        return 0;
    if (child_return)
        *child_return = child;
    if (mainw_return)
        *mainw_return = mainw;
    return mainw->input_context;
}

static XPoint new_spot;

#endif

void CRefreshSpot (void)
{E_
#ifdef USE_XIM
    new_spot.x = -30000;
    new_spot.y = 0;
#endif
}

void cim_send_spot (Window window)
{E_
#ifdef USE_XIM
    XIMStyle input_style;
    input_style = get_input_style ();
    if ((input_style & XIMPreeditPosition)) {
	setPosition (window, &hold_spot);
#ifdef XIM_DEBUG
	printf ("setPosition (window, &hold_spot ->  %d %d\n", hold_spot.x, hold_spot.y);
#endif
	CRefreshSpot ();
    }
#endif
}

static int xim_enabled = 1;

void CDisableXIM(void)
{E_
    xim_enabled = 0;
}

void CEnableXIM(void)
{E_
    xim_enabled = 1;
}

void cim_check_spot_change (Window win)
{E_
#ifdef USE_XIM
    XIC ic = 0;
    if (new_spot.x != hold_spot.x || new_spot.y != hold_spot.y) {
	XVaNestedList preedit_attr;
	if ((ic = get_window_ic (win, 0, 0))) {
#ifdef XIM_DEBUG
	    printf ("cim_send_spot - XIMPreeditPosition %d %d\n", hold_spot.x, hold_spot.y);
#endif
	    preedit_attr = XVaCreateNestedList (0, XNSpotLocation, &hold_spot, NULL);
	    XSetICValues (ic, XNPreeditAttributes, preedit_attr, NULL);
	    XFree (preedit_attr);
	    new_spot.x = hold_spot.x;
	    new_spot.y = hold_spot.y;
	}
    }
#endif
}

KeySym key_sym_xlat (XEvent * ev, char *xlat, int *xlat_len)
{E_
#ifdef USE_XIM
    XIC ic = 0;
    static wchar_t kbuf_wchar[MAX_KBUF];
    Status status_return = 0;
#endif
    static KeySym r;
    static int kbuf_len = 0;
    static KeySym keysym = 0;
    static XComposeStatus compose =
    {NULL, 0};
    static unsigned char kbuf[MAX_KBUF] = "";
    static int valid_keysym = 1;
    if (xlat)
	*xlat = '\0';
    if (ev->type != KeyPress && ev->type != KeyRelease)
	return 0;
/* we mustn't call this twice with the same event */
    if (ev->xkey.x_root == 31234)
	goto no_repeat_call;
    ev->xkey.x_root = 31234;
    keysym = 0;
    kbuf_len = 0;
    if (ev->type == KeyRelease) {
	kbuf_len = XLookupString (&ev->xkey, (char *) kbuf, sizeof (kbuf), &keysym, 0);
	if (!kbuf_len && (keysym >= 0x0100) && (keysym < 0x0800)) {
	    kbuf_len = 1;
	    kbuf[0] = (keysym & 0xFF);
	}
#ifdef USE_XIM
    } else if (xim_enabled && (ic = get_window_ic(ev->xkey.window, 0, 0))) {
        int wlen;
#warning backport this fix:
	wlen = XwcLookupString (ic, &ev->xkey, kbuf_wchar,
			       sizeof (kbuf_wchar) / sizeof (kbuf_wchar[0]), &keysym, &status_return);
	valid_keysym = ((status_return == XLookupKeySym) || (status_return == XLookupBoth));
        if (status_return == XLookupChars || status_return == XLookupBoth) {
            int i;
            kbuf_len = 0;
            for (i = 0; i < wlen; i++) {
                unsigned char *p;
                int l = 0;
                p = font_wchar_to_charenc ((C_wchar_t) kbuf_wchar[i], &l);
                if (kbuf_len + l >= MAX_KBUF)
                    break;
                memcpy (&kbuf[kbuf_len], p, l);
                kbuf_len += l;
            }
        }
#endif
    } else {
	keysym = 0;
	kbuf_len = XLookupString (&ev->xkey, (char *) kbuf, sizeof (kbuf), &keysym, &compose);
	if (!kbuf_len && (keysym >= 0x0100) && (keysym < 0x0800)) {
	    kbuf_len = 1;
	    kbuf[0] = (keysym & 0xFF);
	}
    }
  no_repeat_call:
    if (xlat) {
	if (kbuf_len > 0) {
	    if (kbuf_len > MAX_KBUF)
		kbuf_len = MAX_KBUF;
	    memcpy (xlat, kbuf, kbuf_len);
	}
	*xlat_len = kbuf_len;
    }
    r = ((keysym >= 0x0100) && (keysym < 0x0800)) ? (valid_keysym ? kbuf[0] : 0) : (valid_keysym ? keysym : 0);
    return r;
}


#ifdef USE_XIM

void setSize (CWidget * w, XRectangle * size)
{E_
    size->x = 0;
    size->y = 0;
    size->width = 1600;
    size->height = 1600;
}

void setColor (CWidget * w, unsigned long *fg, unsigned long *bg)
{E_
    *fg = COLOR_BLACK;
    *bg = COLOR_WHITE;
}


static long destroy_input_context (CWidget * w)
{E_
    if (w->input_context) {
        XDestroyIC (w->input_context);
        w->input_context = 0;
    }
    return 0;
}

void IMDestroyCallback (XIM xim, XPointer client_data, XPointer call_data)
{E_
    XRegisterIMInstantiateCallback (CDisplay, NULL, NULL, NULL, IMInstantiateCallback, NULL);
    for_all_widgets ((for_all_widgets_cb_t) destroy_input_context, 0, 0);
    x_server_xim_reported_enabled = 0;
}

/* returns zero on error */
XIMStyle get_input_style (void)
{E_
    int found = 0, i;
    XIMStyle input_style = 0;
    XIMStyles *xim_styles = NULL;
    if (!CIM) {
	if (option_use_xim)
	    xim_print_error ("Trying to get input_style, but Input Method is null.");
	return 0;
    }
    if (XGetIMValues (CIM, XNQueryInputStyle, &xim_styles, NULL) || !xim_styles) {
	xim_print_error ("input method doesn't support any style");
	return 0;
    }
    for (i = 0; !found && i < xim_styles->count_styles; i++) {
	if (xim_styles->supported_styles[i] == (XIMPreeditPosition | XIMStatusNothing)) {
	    found = 1;
	    input_style = xim_styles->supported_styles[i];
	}
    }
    for (i = 0; !found && i < xim_styles->count_styles; i++) {
	if ((xim_styles->supported_styles[i] & XIMPreeditPosition)) {
	    found = 1;
	    input_style = xim_styles->supported_styles[i];
	}
    }
    for (i = 0; !found && i < xim_styles->count_styles; i++) {
	if (xim_styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing)) {
	    found = 1;
	    input_style = xim_styles->supported_styles[i];
	}
    }
    XFree (xim_styles);
    if (found == 0) {
	xim_print_error ("input method doesn't support my preedit type");
	return 0;
    }
    return input_style;
}

long create_input_context (CWidget * w, XIMStyle input_style)
{E_
    XVaNestedList preedit_attr = 0;
    XPoint spot;
    XRectangle rect;
    XIMCallback ximcallback;
    unsigned long fg, bg;
    if (w->kind != C_WINDOW_WIDGET)
	return 0;
    if (w->mainid)
	return 0;
    if (w->input_context)
	return 0;
    if (!CIM)
	return 1;
    if (!input_style)
	return 1;
    ximcallback.callback = IMDestroyCallback;
    ximcallback.client_data = NULL;
    if (input_style & XIMPreeditPosition) {

#ifdef XIM_DEBUG
printf ("XIMPreeditPosition\n");
#endif

	setSize (w, &rect);

#ifdef XIM_DEBUG
printf("CRoot=%lu parentid=%lu\n", (unsigned long) CRoot, (unsigned long) w->parentid);
#endif
	setPosition (w->winid, &spot);
	setColor (w, &fg, &bg);
	preedit_attr = XVaCreateNestedList (0, 
					    XNArea, &rect,
					    XNSpotLocation, &spot,
					    XNForeground, fg,
					    XNBackground, bg,
					    XNFontSet, current_font->f.font_set,
					    NULL);
    }
    w->input_context = XCreateIC (CIM, XNInputStyle, input_style,
				  XNClientWindow, w->winid,
				  XNFocusWindow, w->winid,
				  XNDestroyCallback, &ximcallback,
			       preedit_attr ? XNPreeditAttributes : NULL,
				  preedit_attr,
				  NULL);

#ifdef XIM_DEBUG
printf("w->input_context = %p\n", (void *) w->input_context);
#endif

    if (preedit_attr)
	XFree (preedit_attr);
    if (!w->input_context) {
	xim_print_error ("Failed to create input context for widget %s", w->ident);
	return 1;
    }
    x_server_xim_reported_enabled = 1;
    return 0;
}

long set_status_position (CWidget * w)
{E_
    return 0;
}

static void IMInstantiateCallback (Display * display, XPointer client_data, XPointer call_data)
{E_
    char *p;
    XIMStyle input_style = 0;
    XIMCallback ximcallback;

#ifdef XIM_DEBUG
printf ("IMInstantiateCallback\n");
#endif

    if (x_server_xim_reported_enabled)  /* X must disable before enabling */
	return;

    ximcallback.callback = IMDestroyCallback;
    ximcallback.client_data = NULL;

    /* try with XMODIFIERS env. var. */
    if (CIM == NULL && (p = XSetLocaleModifiers ("")) != NULL && *p)
	CIM = XOpenIM (CDisplay, NULL, NULL, NULL);
    if (CIM == NULL && (p = XSetLocaleModifiers ("@im=control")) != NULL && *p)
	CIM = XOpenIM (CDisplay, NULL, NULL, NULL);
    if (CIM == NULL && (p = XSetLocaleModifiers ("@im=none")) != NULL && *p)
	CIM = XOpenIM (CDisplay, NULL, NULL, NULL);

#ifdef XIM_DEBUG
printf("CIM=%p\n", (void *) CIM);
#endif

    if (!CIM)
	return;

/* got the Input Method, now set up all dialogs */

    XSetIMValues (CIM, XNDestroyCallback, &ximcallback, NULL);

    if (!(input_style = get_input_style ())) {
	XCloseIM (CIM);
	CIM = 0;
    }
    CPushFont ("editor", 0);
    if (for_all_widgets ((for_all_widgets_cb_t) create_input_context, (void *) input_style, 0)) {
	input_style = 0;
	XCloseIM (CIM);
	CIM = 0;
    }
    CPopFont ();
}

#endif

