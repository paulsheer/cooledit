/* bitmapbutton.c - buttons with little pictures on them
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"



extern struct look *look;

void render_rounded_bevel (Window win, int x1, int y1, int x2, int y2, int radius, int thick, int sunken);

CWidget *CDrawBitmapButton (const char *identifier, Window parent, int x, int y,
			    int width, int height, unsigned long fg, unsigned long bg, const unsigned char data[])
{
    XSetWindowAttributes xswa;
    CWidget *w = CSetupWidget (identifier, parent, x, y,
      width + 8, height + 8, C_BITMAPBUTTON_WIDGET, INPUT_BUTTON, bg, 1);
    w->render = render_button;
    xswa.background_pixmap = w->pixmap = XCreateBitmapFromData (CDisplay, w->winid, (char *) data, width, height);
    if (xswa.background_pixmap)
	XChangeWindowAttributes (CDisplay, w->winid, CWBackPixmap, &xswa);
    w->fg = fg;
    w->bg = bg;
    w->options |= WIDGET_TAKES_FOCUS_RING | WIDGET_HOTKEY_ACTIVATES;

    set_hint_pos (x + width + 8 + WIDGET_SPACING, y + height + 8 + WIDGET_SPACING);

    return w;
}

CWidget *CDrawBitmap (const char *identifier, Window parent, int x, int y,
		      int width, int height, unsigned long fg, unsigned long bg, const unsigned char data[])
{
    CWidget *w = CSetupWidget (identifier, parent, x, y,
	     width + 8, height + 8, C_BITMAP_WIDGET, INPUT_EXPOSE, bg, 0);
    Pixmap pixmap;

    pixmap = XCreateBitmapFromData (CDisplay, w->winid, (char *) data, width, height);
    w->pixmap = pixmap;
    
    w->fg = fg;
    w->bg = bg;

    set_hint_pos (x + width + 8 + WIDGET_SPACING, height + 8 + WIDGET_SPACING);

    return w;
}

CWidget *CDrawPixmapButton (const char *identifier, Window parent,
    int x, int y, int width, int height, const char *data[], char start_char)
{
    CWidget *w;
    w = CDrawButton (identifier, parent, x, y, width, height, "");
    CSetBackgroundPixmap (identifier, data, width, height, start_char);
    w->pixmap_mask = CCreateClipMask (data, width, height, ' ');
    return w;
}

#define PICTURE_SWITCH_SIZE 32

Pixmap Cswitchon = 0;
Pixmap Cswitchoff = 0;

/* here ->cursor holds the group (8 bits only)
    and ->keypressed hold if the switch is on or off */

CWidget *CDrawSwitch (const char *identifier, Window parent, int x, int y, int on, const char *label, int group)
{
    int y_button, y_label, h = 0, xt = 0, yt = 0, l;
    CWidget *w;

    if (group & SWITCH_PICTURE_TYPE) {
	l = PICTURE_SWITCH_SIZE;
    } else
	l = (*look->get_switch_size) ();

    if (label) {
	CTextSize (0, &h, label);
	h += TEXT_RELIEF * 2 + 2; 
    }

    if (h > l ) { 
	y_button = y + (h - l) / 2;
	y_label = y;
    } else {
	y_button = y;
	y_label = y + (l - h) / 2;
    }

    w = CSetupWidget (identifier, parent, x, y_button,
	 l, l, C_SWITCH_WIDGET, INPUT_BUTTON, COLOR_FLAT, 1);

    if (group & SWITCH_PICTURE_TYPE)
    if (!Cswitchon) {
	Cswitchon = XCreateBitmapFromData (CDisplay, w->winid, (char *) switchon_bits, PICTURE_SWITCH_SIZE, PICTURE_SWITCH_SIZE);
	Cswitchoff = XCreateBitmapFromData (CDisplay, w->winid, (char *) switchoff_bits, PICTURE_SWITCH_SIZE, PICTURE_SWITCH_SIZE);
    }
    w->fg = COLOR_BLACK;
    w->bg = COLOR_FLAT;
    w->keypressed = on;
    if (label)
	w->label = (char *) strdup (label);
    w->hotkey = find_hotkey (w);
    w->cursor = group & 0x0FFUL;
    w->render = render_switch;
    w->options |= (group & 0xFFFFFF00UL) | WIDGET_TAKES_FOCUS_RING | WIDGET_HOTKEY_ACTIVATES;

    if (label) {
	CWidget *t;
#ifdef NEXT_LOOK
	t = CDrawText (catstrs (identifier, ".label", NULL), parent, x + l, y_label, "%s", label);
#else
	t = CDrawText (catstrs (identifier, ".label", NULL), parent, x + l + WIDGET_SPACING, y_label, "%s", label);
#endif
	t->hotkey = w->hotkey; 		/* underlines the character */
	CGetHintPos (&xt, &yt);
    }

    if (xt < x + l + WIDGET_SPACING)
	xt = x + l + WIDGET_SPACING;
    if (yt < y + l + WIDGET_SPACING)
	yt = y + l + WIDGET_SPACING;
    if (yt < y + h + WIDGET_SPACING)
	yt = y + h + WIDGET_SPACING;

    set_hint_pos (xt, yt);

    return w;
}

/* sets and redraws in the group all except w */
void set_switch_group (CWidget * w, int group, int on)
{
    CWidget *p = w;

    if (!w->cursor)	/* belongs to no group */
	return;
    for (;;) {
	w = CNextFocus (w);
	if (!w)
	    return;
	if ((unsigned long) w == (unsigned long) p)
	    return;
	if (w->cursor == group)
	    if (w->keypressed != on) {
		w->keypressed = on;
		CExpose (w->ident);
	    }
    };
}

void toggle_radio_button (CWidget * w)
{
    if (w->options & RADIO_INVERT_GROUP)
	set_switch_group (w, w->cursor, w->keypressed);
    else
	set_switch_group (w, w->cursor, 0);
    if (w->cursor && w->options & RADIO_ONE_ALWAYS_ON)
	w->keypressed = 1;
    else
	w->keypressed = (w->keypressed == 0);

    /* don't expose, 'cos called from eh_button which renders by itself */
}

void render_switch (CWidget * wdt)
{
    (*look->render_switch) (wdt);
}

void render_rounded_bevel (Window win, int xs1, int ys1, int xs2, int ys2, int radius, int thick, int sunken)
{
    unsigned long cn, cs, cnw, cne, cse;
    int i;
    int x1, y1, x2, y2;

    if (option_low_bandwidth)
	return;

    if ((sunken & 2)) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, xs1, ys1, xs2 - xs1 + 1, ys2 - ys1 + 1);
    }
    sunken &= 1;

    cn = sunken ? color_widget (4) : color_widget (11);
    cs = sunken ? color_widget (11) : color_widget (4);
    cnw = color_widget (14);
    cse = color_widget (2);

    if (sunken) {
	cne = cnw;
	cnw = cse;
	cse = cne;
    }
    cne = color_widget (8);

#define Carc(win,x,y,w,h,a,b) XDrawArc(CDisplay,win,CGC,x,y,w,h,a,b)

    for (x1 = xs1; x1 < xs1 + thick; x1++)
	for (y1 = ys1; y1 < ys1 + thick; y1++) {
	    x2 = x1 + (xs2 - xs1 - thick + 1);
	    y2 = y1 + (ys2 - ys1 - thick + 1);
	    CSetColor (cnw);
	    Carc (win, x1, y1, (radius) * 2, (radius) * 2, 90 * 64, 90 * 64);
	    CSetColor (cse);
	    Carc (win, x2 - radius * 2, y2 - radius * 2, (radius) * 2, (radius) * 2, 270 * 64, 90 * 64);
	    CSetColor (cne);
	    Carc (win, x1, y2 - radius * 2, (radius) * 2, (radius) * 2, 180 * 64, 90 * 64);
	    Carc (win, x2 - radius * 2, y1, (radius) * 2, (radius) * 2, 0, 90 * 64);
	}


    if (radius)
	radius--;
    for (i = 0; i < thick; i++) {
	CSetColor (cn);
	CLine (win, xs1 + i, ys1 + radius, xs1 + i, ys2 - radius);
	CLine (win, xs1 + radius, ys1 + i, xs2 - radius, ys1 + i);
	CSetColor (cs);
	CLine (win, xs2 - radius, ys2 - i, xs1 + radius, ys2 - i);
	CLine (win, xs2 - i, ys1 + radius, xs2 - i, ys2 - radius);
	CSetColor (color_widget (15));
	if (sunken)
	    XDrawPoint (CDisplay, win, CGC, xs2 - i - (radius + 1) * 300 / 1024, ys2 - i - (radius + 1) * 300 / 1024);
	else
	    XDrawPoint (CDisplay, win, CGC, xs1 + i + (radius + 1) * 300 / 1024, ys1 + i + (radius + 1) * 300 / 1024);
    }


    CSetColor (COLOR_BLACK);
}
