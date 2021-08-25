/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* unicodedialog.c - draw a unicode selector
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <X11/Xatom.h>
#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"

#include "coollocal.h"
#include "font.h"

#include "edit.h"
#include "editcmddef.h"


#define MID_X 20
#define MID_Y 20

#define COL_SPACING             ((FONT_MEAN_WIDTH * 5 / 3) > FONT_PIX_PER_LINE ? (FONT_MEAN_WIDTH * 5 / 3) : FONT_PIX_PER_LINE)
#define UNICODE_INCOMPLETE_DRAW (1UL << 30)

extern struct look *look;
Window find_mapped_window (Window w);

unsigned char *wcrtomb_wchar_to_utf8 (C_wchar_t c);

int last_unichar_left = 0;
int last_unichar_right = 0;
int last_unichar_focussed = 0;

static void render_unicode (CWidget * wdt, int force_full)
{E_
    int w, h, isfocussed, i, j, col_spacing;
    Window win, temp_win;

    CPushFont ("editor", 0);
    win = wdt->pixmap;
    isfocussed = (wdt->winid == CGetFocus ());

    CSetColor (COLOR_WHITE);
    CRectangle (win, 2, 2, wdt->width - 4, wdt->height - 4);

    col_spacing = COL_SPACING;

    wdt->options &= ~UNICODE_INCOMPLETE_DRAW;

    for (j = 0; j < 16; j++) {
	for (i = 0; i < 17; i++) {
	    C_wchar_t wc;
	    wc = i + j * 16 + (wdt->cursor & ~0xFF);
	    w = (col_spacing + 5) * i + 5;
	    h = (FONT_PIX_PER_LINE + 5) * j + 5;
	    CSetBackgroundColor (COLOR_WHITE);
	    if (i == 16) {
		CSetColor (COLOR_FLAT);
		CImageText (win, w, h + FONT_BASE_LINE, "0123456789ABCDEF" + j, 1);
	    } else {
                if (force_full || !CCheckWindowEvent (0, KeyPressMask, 1)) {
/* the part inside this block takes a long time to render due to having to load fonts and possibly
scale larger glyphs */
	            int l;
		    l = FONT_PER_CHAR (wc);
		    if (l) {
		        CSetColor (COLOR_BLACK);
		        CImageTextWC (win, w, h + FONT_BASE_LINE, 0, &wc, 1);
		    } else {
		        CSetColor (COLOR_FLAT);
		        l = FONT_MEAN_WIDTH;
		        CRectangle (win, w, h, l, FONT_PIX_PER_LINE);
		    }
		    if (wc == wdt->cursor) {
		        CSetColor (color_palette (18));
		        CBox (win, w - 2, h - 2, l + 4, FONT_PIX_PER_LINE + 4);
		    }
	        } else {
                    wdt->options |= UNICODE_INCOMPLETE_DRAW;
	        }
	    }
	}
    }
    {
	char c[10];
	sprintf (c, "%06X", (unsigned int) wdt->cursor);
	w = (col_spacing + 5) * 0 + 5;
	h = (FONT_PIX_PER_LINE + 5) * j + 5;
	CSetBackgroundColor (COLOR_WHITE);
	CSetColor (color_palette (1));
	CImageText (win, w, h + FONT_BASE_LINE, c, strlen (c));
    }
    for (i = 3; i < 16; i++) {
	w = (col_spacing + 5) * i + 5;
	h = (FONT_PIX_PER_LINE + 5) * j + 5;
	CSetBackgroundColor (COLOR_WHITE);
	CSetColor (COLOR_FLAT);
	CImageText (win, w, h + FONT_BASE_LINE, "0123456789ABCDEF" + i, 1);
    }

    w = wdt->width;
    h = wdt->height;

    temp_win = wdt->winid;
    wdt->winid = win;
    (*look->render_textbox_tidbits) (wdt, isfocussed);
    wdt->winid = temp_win;
    XCopyArea(CDisplay, win, wdt->winid, CGC, 0, 0, w, h, 0, 0);
    CPopFont ();
    return;
}

/*
   This will reallocate a previous draw of the same identifier.
   so you can draw the same widget over and over without flicker
 */
CWidget *CDrawUnicodeInput (const char *identifier, Window parent, int x, int y, C_wchar_t start_char)
{E_
    CWidget *wdt;
    int w, h;

    CPushFont ("editor", 0);

    w = (COL_SPACING + 5) * (16 + 1) - FONT_MEAN_WIDTH + 5;
    h = (FONT_PIX_PER_LINE + 5) * (16 + 1) + 5;

    set_hint_pos (x + w + WIDGET_SPACING, y + h + WIDGET_SPACING);

    wdt = CSetupWidget (identifier, parent, x, y, w, h, C_UNICODE_WIDGET, INPUT_KEY, COLOR_WHITE, 1);
    wdt->cursor = start_char;
    wdt->pixmap = XCreatePixmap (CDisplay, wdt->winid, w, h, CDepth);

    xdnd_set_type_list (CDndClass, wdt->winid, xdnd_typelist_send[DndText]);
    CPopFont ();
    return wdt;
}

int eh_unicode (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    int handled = 0;
    int i, j;
    long cursor;
    cursor = w->cursor;
    switch (xevent->type) {
    case ButtonPress:
	resolve_button (xevent, cwevent);
	CFocus (w);
	CPushFont ("editor", 0);
	i = (xevent->xbutton.x - 5) / (COL_SPACING + 5);
	j = (xevent->xbutton.y - 5) / (FONT_PIX_PER_LINE + 5);
	if (i < 16 && j < 16 && i >= 0 && j >= 0)
	    w->cursor = i + j * 16 + (w->cursor & ~0xFF);
	CPopFont ();
	break;
    case FocusIn:
    case FocusOut:
	render_unicode (w, 1);
	return 0;
    case Expose:
	if (!xevent->xexpose.count)
	    render_unicode (w, 1);
	return 0;
    case ButtonRelease:
    case EnterNotify:
    case MotionNotify:
    case LeaveNotify:
	return 0;
	break;
    case KeyPress:
	cwevent->ident = w->ident;
	cwevent->state = xevent->xkey.state;
	switch (cwevent->command) {
	case CK_Down:
	    if (w->cursor <= FONT_LAST_UNICHAR - 16)
		w->cursor += 16;
	    handled = 1;
	    break;
	case CK_Up:
	    if (w->cursor >= 16)
		w->cursor -= 16;
	    handled = 1;
	    break;
	case CK_Right:
	    if (w->cursor <= FONT_LAST_UNICHAR - 1)
		w->cursor += 1;
	    handled = 1;
	    break;
	case CK_Left:
	    if (w->cursor >= 1)
		w->cursor -= 1;
	    handled = 1;
	    break;
	case CK_Page_Up:
	    if (w->cursor >= 256)
		w->cursor -= 256;
	    handled = 1;
	    break;
	case CK_Page_Down:
	    if (w->cursor <= FONT_LAST_UNICHAR - 256)
		w->cursor += 256;
	    handled = 1;
	    break;
        case CK_Begin_Page:
        case CK_Home_Highlight:        /* give some intuitive options */
            w->cursor = 0;
	    handled = 1;
	    break;
        case CK_End_Page:
        case CK_End_Highlight:        /* give some intuitive options */
            w->cursor = FONT_LAST_UNICHAR;
	    handled = 1;
	    break;
	case CK_Home:
            if (w->cursor >= 256 * 16)
	        w->cursor -= 256 * 16;
	    handled = 1;
	    break;
	case CK_End:
	    if (w->cursor <= FONT_LAST_UNICHAR - 256 * 16)
		w->cursor += 256 * 16;
	    handled = 1;
	    break;
	}
	break;
    }
    if (w->cursor != cursor || (w->options & UNICODE_INCOMPLETE_DRAW))
	render_unicode (w, w->cursor == cursor);
    w->keypressed |= handled;
    return handled;
}

long CUnicodeDialog (Window in, int x, int y, char *heading)
{E_
    Window win;
    CEvent cwevent;
    CState s;
    CWidget *w;
    long result = -1;
    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);
    CBackupState (&s);
    CDisable ("*");
    if (heading)
	win = CDrawHeadedDialog ("_unicode", in, x, y, heading);
    else
	win = CDrawDialog ("_unicode", in, x, y);
    CGetHintPos (&x, &y);
    CDrawUnicodeInput ("_unicode.box1", win, x, y, last_unichar_left);
    CGetHintPos (&x, 0);
    CDrawUnicodeInput ("_unicode.box2", win, x, y, last_unichar_right);
    CSetSizeHintPos ("_unicode");
    CMapDialog ("_unicode");
    if (last_unichar_focussed)
	CFocus (CIdent ("_unicode.box2"));
    else
	CFocus (CIdent ("_unicode.box1"));
    do {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_unicode"))
	    break;
	if (cwevent.double_click)
	    cwevent.command = CK_Enter;
    } while (cwevent.command != CK_Cancel && cwevent.command != CK_Enter);
    if ((w = CIdent ("_unicode.box1"))) {
	if (CGetFocus () == w->winid) {
	    last_unichar_focussed = 0;
	    if (cwevent.command == CK_Enter)
		result = (C_wchar_t) w->cursor;
	}
	last_unichar_left = (int) w->cursor;
    }
    if ((w = CIdent ("_unicode.box2"))) {
	if (CGetFocus () == w->winid) {
	    last_unichar_focussed = 1;
	    if (cwevent.command == CK_Enter)
		result = (C_wchar_t) w->cursor;
	}
	last_unichar_right = (int) w->cursor;
    }
    CDestroyWidget ("_unicode");
    CRestoreState (&s);
    return result;
}

unsigned char *CGetUnichar (Window in, char *heading)
{E_
    long r;
    r = CUnicodeDialog (in, MID_X, MID_Y, heading);
    if (r == -1)
	return 0;
    return wcrtomb_wchar_to_utf8 (r);
}

