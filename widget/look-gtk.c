/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* look-gtk.c - look 'n feel type: GTK
   Copyright (C) 1996-2022 Paul Sheer
 */


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


extern struct look *look;

const char *get_default_widget_font (void);

extern int look_cool_search_replace_dialog (Window parent, int x, int y, CStr *search_text, CStr *replace_text, CStr *arg_order, const char *heading, int option);

extern CWidget *look_cool_draw_file_list (const char *identifier, Window parent, int x, int y,
					 int width, int height, int line, int column,
					 struct file_entry *directentry, long options);

extern CWidget *look_cool_redraw_file_list (const char *identifier,
						  struct file_entry *directentry, int preserve);

extern struct file_entry *look_cool_get_file_list_line (CWidget * w, int line);

Window find_mapped_window (Window w);

char *look_cool_get_file_or_dir (Window parent, int x, int y,
       char *host, const char *dir, const char *file, const char *label, int options);

void look_cool_draw_browser (const char *ident, Window parent, int x, int y,
		   char *host, const char *dir, const char *file, const char *label);


/* }}} file browser stuff */

int find_menu_hotkey (struct menu_item m[], int this, int num);

/* outermost bevel */
#define BEVEL_MAIN	2
/* next-outermost bevel */
#define BEVEL_IN 	2
#define BEVEL_OUT	2
/* between items, and between items and next-outermost bevel */
#define SPACING		2
/* between items rectangle and text */
#define RELIEF		3

#define S		SPACING
/* between window border and items */
#define O		(BEVEL_OUT + SPACING)
/* total height of an item */

/* size of bar item */
#define BAR_HEIGHT	4

#define H		(FONT_PIX_PER_LINE + RELIEF * 2)

#define B		BAR_HEIGHT

static void look_gtk_get_menu_item_extents (int n, int j, struct menu_item m[], int *border, int *relief, int *y1, int *y2)
{E_
    int i, n_items = 0, n_bars = 0;

    *border = O;
    *relief = RELIEF;

    if (!n || j < 0) {
	*y1 = O;
	*y2 = *y1 + H;
    } else {
	int not_bar;
	not_bar = (m[j].text[2] != '\0');
	for (i = 0; i < j; i++)
	    if (m[i].text[2])
		n_items++;
	    else
		n_bars++;
	*y1 = O + n_items * (H + S) + n_bars * (B + S) + (not_bar ? 0 : 2);
	*y2 = *y1 + (not_bar ? H : (B - 4));
    }
}

unsigned long bevel_background_color = 1;

static void look_gtk_menu_draw (Window win, int w, int h, struct menu_item m[], int n, int light)
{E_
    int i, y1, y2, offset = 0;
    static int last_light = 0, last_n = 0;
    static Window last_win = 0;

    render_bevel (win, 0, 0, w - 1, h - 1, BEVEL_MAIN, 0);
#if 0
    render_bevel (win, BEVEL_IN, BEVEL_IN, w - 1 - BEVEL_IN, h - 1 - BEVEL_IN, BEVEL_OUT - BEVEL_IN, 1);
#endif

    if (last_win == win && last_n != n) {
	XClearWindow (CDisplay, win);
    } else if (last_light >= 0 && last_light < n) {
	int border, relief;
	look_gtk_get_menu_item_extents (n, last_light, m, &border, &relief, &y1, &y2);
	CSetColor (COLOR_FLAT);
	CRectangle (win, O - 1, y1 - 1, w - O * 2 + 2, y2 - y1 + 2);
    }
    last_win = win;
    last_n = n;
    CPushFont ("widget", 0);
    for (i = 0; i < n; i++) {
	int border, relief;
	look_gtk_get_menu_item_extents (n, i, m, &border, &relief, &y1, &y2);
	if (i == light && m[i].text[2]) {
	    offset = 1;
	    bevel_background_color = color_widget (14);
	    render_bevel (win, O - 1, y1 - 1, w - O, y2, 2, 2);
	    bevel_background_color = COLOR_FLAT;
	} else {
	    if (!(m[i].text[2])) {
		CSetColor (color_widget (9));
		CLine (win, O, y1 - 1, w - O, y1 - 1);
		CSetColor (color_widget (14));
		CLine (win, O, y1, w - O, y1);
	    }
	    offset = 0;
	}
	if (m[i].text[2]) {

	    char *u;
	    u = strrchr (m[i].text, '\t');
	    if (u)
		*u = 0;
	    CSetColor (COLOR_BLACK);
	    if (m[i].hot_key == '~')
		m[i].hot_key = find_menu_hotkey (m, i, n);
	    if (i == light)
		CSetBackgroundColor (color_widget (14));
	    else
		CSetBackgroundColor (COLOR_FLAT);
	    drawstring_xy_hotkey (win, RELIEF + O - offset,
			  RELIEF + y1 - offset, m[i].text, m[i].hot_key);
	    if (u) {
		drawstring_xy (win, RELIEF + O + (w - (O + RELIEF) * 2 - CImageStringWidth (u + 1)) - offset,
			       RELIEF + y1 - offset, u + 1);
		*u = '\t';
	    }
	}
    }
    last_light = light;
    CPopFont ();
}

static void look_gtk_render_menu_button (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    int x = 0, y = 0;

    Window win = wdt->winid;

    if (wdt->disabled)
	goto disabled;
    if (wdt->options & BUTTON_PRESSED || wdt->droppedmenu[0]) {
	render_bevel (win, x, y, x + w - 1, y + h - 1, 2, 0);
    } else {
  disabled:
	CSetColor (COLOR_FLAT);
	XDrawRectangle (CDisplay, win, CGC, x, y, w - 1, h - 1);
	XDrawRectangle (CDisplay, win, CGC, x + 1, y + 1, w - 3, h - 3);
    }

    if (!wdt->label)
	return;
    if (!(*(wdt->label)))
	return;
    CSetColor (COLOR_BLACK);
    CPushFont ("widget", 0);
    CSetBackgroundColor (COLOR_FLAT);
    drawstring_xy_hotkey (win, x + 2 + BUTTON_RELIEF, y + 2 + BUTTON_RELIEF, wdt->label, wdt->hotkey);
    CPopFont ();
}

static void look_gtk_render_button (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    int x = 0, y = 0;
    XGCValues gcv;

    Window win = wdt->winid;
#define BUTTON_BEVEL 2

    if (wdt->pixmap_mask) {
	gcv.clip_mask = wdt->pixmap_mask;
	XChangeGC (CDisplay, CGC, GCClipMask, &gcv);
    }

    if (wdt->disabled)
	goto disabled;
    if (wdt->options & BUTTON_PRESSED) {
	bevel_background_color = color_widget (10);
	render_bevel (win, x, y, x + w - 1, y + h - 1, BUTTON_BEVEL, 3);
	bevel_background_color = COLOR_FLAT;
	CSetBackgroundColor (color_widget (10));
    } else if (wdt->options & BUTTON_HIGHLIGHT) {
	bevel_background_color = color_widget (14);
	render_bevel (win, x, y, x + w - 1, y + h - 1, 2, 2);
	bevel_background_color = COLOR_FLAT;
	CSetBackgroundColor (color_widget (14));
    } else {
      disabled:
	render_bevel (win, x, y, x + w - 1, y + h - 1, BUTTON_BEVEL, 2);
	CSetBackgroundColor (COLOR_FLAT);
    }

    if (wdt->label) {
	if ((*(wdt->label))) {
	    CSetColor (COLOR_BLACK);
	    CPushFont ("widget", 0);
	    drawstring_xy_hotkey (win, x + 2 + BUTTON_RELIEF, y + 2 + BUTTON_RELIEF, wdt->label,
				  wdt->hotkey);
	    CPopFont ();
	}
    }
    if (wdt->pixmap_mask) {
	gcv.clip_mask = 0;
	XChangeGC (CDisplay, CGC, GCClipMask, &gcv);
    }
}

static void look_gtk_render_bar (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    CSetColor (color_widget (9));
    CLine (win, 0, 0, w - 1, 0);
    CSetColor (color_widget (15));
    CLine (win, 0, h - 1, w - 1, h - 1);
}

void look_gtk_render_sunken_bevel (Window win, int x1, int y1, int x2, int y2, int thick,
				   int sunken)
{E_
    int i;

    CSetColor (color_widget (9));
    CLine (win, x1, y1, x2, y1);
    CLine (win, x1, y1 + 1, x1, y2);

    if (thick > 1) {
	CSetColor (color_widget (0));
	CLine (win, x1 + 1, y1 + 1, x2 - 1, y1 + 1);
	CLine (win, x1 + 1, y1 + 2, x1 + 1, y2 - 1);
    }

    CSetColor (color_widget (15));
    CLine (win, x2, y1 + 1, x2, y2);
    CLine (win, x1 + 1, y2, x2 - 1, y2);

    if (thick > 1) {
	CSetColor (bevel_background_color == COLOR_WHITE ? COLOR_FLAT : bevel_background_color);
	CLine (win, x2 - 1, y1 + 2, x2 - 1, y2 - 1);
	CLine (win, x1 + 2, y2 - 1, x2 - 2, y2 - 1);
    }

    if (thick > 2) {
	CSetColor (bevel_background_color);
	for (i = 2; i < thick; i++) {
	    CLine (win, x1 + i, y1 + i, x2 - 1 - i, y1 + i);
	    CLine (win, x1 + i, y1 + i + 1, x1 + i, y2 - 1 - i);
	    CLine (win, x2 - i, y1 + i, x2 - i, y2 - i);
	    CLine (win, x1 + i, y2 - i, x2 - i - 1, y2 - i);
	}
    }

    if ((sunken & 2)) {
	CSetColor (bevel_background_color);
	CRectangle (win, x1 + thick, y1 + thick, x2 - x1 - 2 * thick + 1, y2 - y1 - 2 * thick + 1);
    }
}

static void look_gtk_render_raised_bevel (Window win, int x1, int y1, int x2, int y2, int thick,
					  int sunken)
{E_
    int i;

    if (bevel_background_color == 1)
	bevel_background_color = COLOR_FLAT;

    x2--;
    y2--;

    CSetColor (color_widget (15));
    CLine (win, x1, y1, x1, y2);
    CLine (win, x1 + 1, y1, x2, y1);
    if (thick > 1) {
	CLine (win, x1 + 1, y2, x1 + 1, y2);
	CLine (win, x2, y1 + 1, x2, y1 + 1);

	CSetColor (color_widget (9));
	CLine (win, x1 + 2, y2, x2 - 1, y2);
	CLine (win, x2, y1 + 2, x2, y2);
    }

    CSetColor (color_widget (0));
    CLine (win, x1, y2 + 1, x2, y2 + 1);
    CLine (win, x2 + 1, y1, x2 + 1, y2 + 1);

    if (thick > 1) {
	CSetColor (bevel_background_color);
	CLine (win, x1 + 1, y1 + 1, x1 + 1, y2 - 1);
	CLine (win, x1 + 1, y1 + 1, x2 - 1, y1 + 1);
    }

    x2++;
    y2++;

    if (thick > 2) {
	for (i = 2; i < thick; i++) {
	    CLine (win, x1 + i, y1 + i, x2 - 1 - i, y1 + i);
	    CLine (win, x1 + i, y1 + i + 1, x1 + i, y2 - 1 - i);
	    CLine (win, x2 - i, y1 + i, x2 - i, y2 - i);
	    CLine (win, x1 + i, y2 - i, x2 - i - 1, y2 - i);
	}
    }
    if ((sunken & 2)) {
	CSetColor (bevel_background_color);
	CRectangle (win, x1 + thick, y1 + thick, x2 - x1 - 2 * thick + 1, y2 - y1 - 2 * thick + 1);
    }
}

static void look_gtk_draw_hotkey_understroke (Window win, int x, int y, int hotkey)
{E_
    CLine (win, x, y + 1, x + FONT_PER_CHAR (hotkey) - 1, y + 1);
}

static void look_gtk_render_text (CWidget * wdt)
{E_
    Window win = wdt->winid;
    char text[1024], *p, *q;
    int hot, y, w = wdt->width, center = 0;

    CPushFont ("widget", 0);

    CSetColor (COLOR_FLAT);
    CRectangle (win, 0, 0, w - 1, wdt->height - 1);
    CSetColor (COLOR_BLACK);

    hot = wdt->hotkey;		/* a letter that needs underlining */
    y = 1;			/* bevel */
    q = wdt->text.data;

    CSetBackgroundColor (COLOR_FLAT);
    for (;;) {
	p = strchr (q, '\n');
	if (!p) {	/* last line */
	    if (wdt->options & TEXT_CENTRED)
		center = (wdt->width - (TEXT_RELIEF + 1) * 2 - CImageTextWidth (q, strlen (q))) / 2;
	    drawstring_xy_hotkey (win, TEXT_RELIEF + 1 + center,
		 TEXT_RELIEF + y, q, hot);
	    break;
	} else {
	    int l;
	    l = min (1023, (unsigned long) p - (unsigned long) q);
	    memcpy (text, q, l);
	    text[l] = 0;
	    if (wdt->options & TEXT_CENTRED)
		center = (wdt->width - (TEXT_RELIEF + 1) * 2 - CImageTextWidth (q, l)) / 2;
	    drawstring_xy_hotkey (win, TEXT_RELIEF + 1 + center,
		 TEXT_RELIEF + y, text, hot);
	}
	y += FONT_PIX_PER_LINE;
	hot = 0;	/* only for first line */
	q = p + 1;	/* next line */
    }

    CPopFont ();

    return;
}

static void look_gtk_render_window (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;

    Window win = wdt->winid;

    if (wdt->options & WINDOW_NO_BORDER)
	return;

    if (wdt->position & WINDOW_RESIZABLE) {
	CSetColor (color_widget (13));
	CLine (win, w - 4, h - 31, w - 31, h - 4);
	CLine (win, w - 4, h - 21, w - 21, h - 4);
	CLine (win, w - 4, h - 11, w - 11, h - 4);

	CLine (win, w - 4, h - 32, w - 32, h - 4);
	CLine (win, w - 4, h - 22, w - 22, h - 4);
	CLine (win, w - 4, h - 12, w - 12, h - 4);

	CSetColor (color_widget (3));
	CLine (win, w - 4, h - 27, w - 27, h - 4);
	CLine (win, w - 4, h - 17, w - 17, h - 4);
	CLine (win, w - 4, h -  7, w -  7, h - 4);

	CLine (win, w - 4, h - 28, w - 28, h - 4);
	CLine (win, w - 4, h - 18, w - 18, h - 4);
	CLine (win, w - 4, h -  8, w -  8, h - 4);
    }
    render_bevel (win, 0, 0, w - 1, h - 1, 2, 0);
    if (CRoot != wdt->parentid)
	if (win == CGetFocus ())
	    render_bevel (win, 4, 4, w - 5, h - 5, 3, 1);
}

static void look_gtk_render_vert_scrollbar (Window win, int x, int y, int w, int h, int pos, int prop, int pos2, int prop2, int flags)
{E_
    int l = h - 10 * w / 3 - 5;

    render_bevel (win, 0, 0, w - 1, h - 1, 2, 1);
    CSetColor (COLOR_FLAT);
    CRectangle (win, 2, w + 2 * w / 3 + 2, w - 4, (l - 5) * pos / 65535);
    CRectangle (win, 2, w + 2 * w / 3 + 3 + l * (prop + pos) / 65535, w - 4, h - 1 - w - 2 * w / 3 - (w + 2 * w / 3 + 4 + l * (prop + pos) / 65535));

    if (flags & 32) {
	bevel_background_color = ((flags & 15) == 1) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, 2, 2, w - 3, w + 1, 2, 2);
	bevel_background_color = ((flags & 15) == 2) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, 2, w + 2, w - 3, w + 2 * w / 3 + 1, 2, 2);
	bevel_background_color = ((flags & 15) == 4) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, 2, h - 2 - w, w - 3, h - 3, 2, 2);
	bevel_background_color = ((flags & 15) == 5) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, 2, h - 2 - w - 2 * w / 3, w - 3, h - 3 - w, 2, 2);
	bevel_background_color = ((flags & 15) == 3) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, 2, w + 2 * w / 3 + 2 + (l - 5) * pos / 65535, w - 3, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, 2, 2);
	bevel_background_color = COLOR_FLAT;
    } else {
	render_bevel (win, 2, 2, w - 3, w + 1, 2, 2 | ((flags & 15) == 1));
	render_bevel (win, 2, w + 2, w - 3, w + 2 * w / 3 + 1, 2, 2 | ((flags & 15) == 2));
	render_bevel (win, 2, h - 2 - w, w - 3, h - 3, 2, 2 | ((flags & 15) == 4));
	render_bevel (win, 2, h - 2 - w - 2 * w / 3, w - 3, h - 3 - w, 2, 2 | ((flags & 15) == 5));
	if ((flags & 15) == 3) {
	    CSetColor (color_widget (5));
	    XDrawRectangle (CDisplay, win, CGC, 4, w + 2 * w / 3 + 4 + (l - 5) * pos2 / 65535, w - 10, 2 + (l - 5) * prop2 / 65535);
	}
	render_bevel (win, 2, w + 2 * w / 3 + 2 + (l - 5) * pos / 65535, w - 3, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, 2, 2 | ((flags & 15) == 3));
    }
}

static void look_gtk_render_hori_scrollbar (Window win, int x, int y, int h, int w, int pos, int prop, int flags)
{E_
    int l = h - 10 * w / 3 - 5, k;
    k = (l - 5) * pos / 65535;

    render_bevel (win, 0, 0, h - 1, w - 1, 2, 1);
    CSetColor (COLOR_FLAT);

    CRectangle (win, w + 2 * w / 3 + 2, 2, (l - 5) * pos / 65535, w - 4);
    CRectangle (win, w + 2 * w / 3 + 3 + l * (prop + pos) / 65535, 2, h - 1 - w - 2 * w / 3 - (w + 2 * w / 3 + 4 + l * (prop + pos) / 65535), w - 4);

    if (flags & 32) {
	bevel_background_color = ((flags & 15) == 1) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, 2, 2, w + 1, w - 3, 2, 2);
	bevel_background_color = ((flags & 15) == 2) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, w + 2, 2, w + 2 * w / 3 + 1, w - 3, 2, 2);
	bevel_background_color = ((flags & 15) == 4) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, h - 2 - w, 2, h - 3, w - 3, 2, 2);
	bevel_background_color = ((flags & 15) == 5) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, h - 2 - w - 2 * w / 3, 2, h - 3 - w, w - 3, 2, 2);
	bevel_background_color = ((flags & 15) == 3) ? color_widget (14) : COLOR_FLAT;
	render_bevel (win, w + 2 * w / 3 + 2 + k, 2, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, w - 3, 2, 2);
	bevel_background_color = COLOR_FLAT;
    } else {
	render_bevel (win, 2, 2, w + 1, w - 3, 2, 2 | ((flags & 15) == 1));
	render_bevel (win, w + 2, 2, w + 2 * w / 3 + 1, w - 3, 2, 2 | ((flags & 15) == 2));
	render_bevel (win, h - 2 - w, 2, h - 3, w - 3, 2, 2 | ((flags & 15) == 4));
	render_bevel (win, h - 2 - w - 2 * w / 3, 2, h - 3 - w, w - 3, 2, 2 | ((flags & 15) == 5));
	render_bevel (win, w + 2 * w / 3 + 2 + k, 2, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, w - 3, 2, 2 | ((flags & 15) == 3));
    }
}

static void look_gtk_render_scrollbar (CWidget * wdt)
{E_
    int flags = wdt->options;
    if (!wdt)
	return;
    if (wdt->numlines < 0)
	wdt->numlines = 0;
    if (wdt->firstline < 0)
	wdt->firstline = 0;
    if (wdt->firstline > 65535)
	wdt->firstline = 65535;
    if (wdt->firstline + wdt->numlines >= 65535)
	wdt->numlines = 65535 - wdt->firstline;
    if (wdt->kind == C_VERTSCROLL_WIDGET) {
	look_gtk_render_vert_scrollbar (wdt->winid,
			      wdt->x, wdt->y,
			      wdt->width, wdt->height,
			      wdt->firstline, wdt->numlines, wdt->search_start, wdt->search_len, flags);
    } else
	look_gtk_render_hori_scrollbar (wdt->winid,
			      wdt->x, wdt->y,
			      wdt->width, wdt->height,
			      wdt->firstline, wdt->numlines, flags);
    if (wdt->scroll_bar_extra_render)
	(*wdt->scroll_bar_extra_render) (wdt);
}

/*
   Which scrollbar button was pressed: 3 is the middle button ?
 */
static int look_gtk_which_scrollbar_button (int bx, int by, CWidget * wdt)
{E_
    int w, h;
    int pos = wdt->firstline;
    int prop = wdt->numlines;
    int l;

    if (wdt->kind == C_VERTSCROLL_WIDGET) {
	w = wdt->width;
	h = wdt->height;
    } else {
	int t = bx;
	bx = by;
	by = t;
	w = wdt->height;
	h = wdt->width;
    }
    l = h - 10 * w / 3 - 5;

    if (inbounds (bx, by, 2, 2, w - 3, w + 1))
	return 1;
    if (inbounds (bx, by, 2, w + 2, w - 3, w + 2 * w / 3 + 1))
	return 2;
    if (inbounds (bx, by, 2, h - 2 - w, w - 3, h - 3))
	return 4;
    if (inbounds (bx, by, 2, h - 2 - w - 2 * w / 3, w - 3, h - 3 - w))
	return 5;
    if (inbounds (bx, by, 2, w + 2 * w / 3 + 2 + (l - 5) * pos / 65535, w - 3, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535))
	return 3;
    return 0;
}

extern int look_cool_scrollbar_handler (CWidget * w, XEvent * xevent, CEvent * cwevent);

static void look_gtk_init_scrollbar_icons (CWidget * w)
{E_
    return;
}

static int look_gtk_get_scrollbar_size (int type)
{E_
    if (type == C_HORISCROLL_WIDGET)
	return 13;
    return 20;
}

static void look_gtk_get_button_color (XColor * color, int i)
{E_
    color->red = i * 65535 / 15;
    color->green = i * 65535 / 15;
    color->blue = i * 65535 / 15;
    color->flags = DoRed | DoBlue | DoGreen;

#if 0
    double r, g, b, min_wc;

    r = 0.6;
    g = 0.6;
    b = 0.6;

    min_wc = min (r, min (g, b));

    color->red = (float) 65535 *my_pow ((float) i / 20, r) * my_pow (0.75, -min_wc);
    color->green = (float) 65535 *my_pow ((float) i / 20, g) * my_pow (0.75, -min_wc);
    color->blue = (float) 65535 *my_pow ((float) i / 20, b) * my_pow (0.75, -min_wc);
    color->flags = DoRed | DoBlue | DoGreen;
#endif
}

static int look_gtk_get_default_interwidget_spacing (void)
{E_
    return 2;
}

extern int look_cool_window_handler (CWidget * w, XEvent * xevent, CEvent * cwevent);

extern Pixmap Cswitchon;
extern Pixmap Cswitchoff;

static void look_gtk_render_switch (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    if (wdt->options & BUTTON_HIGHLIGHT)
	bevel_background_color = color_widget (14);
    CSetColor (bevel_background_color);
    CRectangle (win, 0, 0, w, h);
    render_bevel (win, w / 2 - 5, h / 2 - 5, w / 2 + 4, h / 2 + 4, 2, wdt->keypressed ? 1 : 0);
    bevel_background_color = COLOR_FLAT;
}

extern int edit_normal_background_color;

static void look_gtk_edit_render_tidbits (CWidget * wdt)
{E_
    int isfocussed;
    int w = wdt->width, h = wdt->height;
    Window win;

    win = wdt->winid;
    isfocussed = (win == CGetFocus ());
    bevel_background_color = edit_normal_background_color;
    if (isfocussed) {
	render_bevel (win, 1, 1, w - 2, h - 2, 2, 1);	/*most outer border bevel */
	CSetColor (color_widget (0));
	XDrawRectangle(CDisplay, win, CGC, 0, 0, w - 1, h - 1);
    } else {
	render_bevel (win, 0, 0, w - 1, h - 1, 3, 1);	/*most outer border bevel */
    }
    bevel_background_color = COLOR_FLAT;
    CSetColor (edit_normal_background_color);
    CLine (CWindowOf (wdt), 3, 3, 3, CHeightOf (wdt) - 4);
}

extern CWidget *look_cool_draw_exclam_cancel_button (char *ident, Window win, int x, int y);

extern CWidget *look_cool_draw_tick_cancel_button (char *ident, Window win, int x, int y);

extern CWidget *look_cool_draw_cross_cancel_button (char *ident, Window win, int x, int y);

extern int option_text_bg_normal;

static void look_gtk_render_fielded_textbox_tidbits (CWidget * w, int isfocussed)
{E_
    bevel_background_color = COLOR_WHITE;
    if (isfocussed) {
	render_bevel (w->winid, 1, 1, w->width - 2, w->height - 2, 2, 1);	/*most outer border bevel */
	CSetColor (color_widget (0));
	XDrawRectangle (CDisplay, w->winid, CGC, 0, 0, w->width - 1, w->height - 1);
    } else {
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 3, 1);	/*most outer border bevel */
    }
    bevel_background_color = COLOR_FLAT;
    CSetColor (color_palette (option_text_bg_normal));
    CLine (w->winid, 3, 3, 3, w->height - 4);
}

static void look_gtk_render_textbox_tidbits (CWidget * w, int isfocussed)
{E_
    bevel_background_color = COLOR_WHITE;
    if (isfocussed) {
	render_bevel (w->winid, 1, 1, w->width - 2, w->height - 2, 2, 1);	/*most outer border bevel */
	CSetColor (color_widget (0));
	XDrawRectangle (CDisplay, w->winid, CGC, 0, 0, w->width - 1, w->height - 1);
    } else {
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 3, 1);	/*most outer border bevel */
    }
    bevel_background_color = COLOR_FLAT;
}

static void look_gtk_render_passwordinput_tidbits (CWidget * wdt, int isfocussed)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    bevel_background_color = COLOR_WHITE;
    if ((win == CGetFocus ())) {
	render_bevel (win, 1, 1, w - 2, h - 2, 2, 1);	/*most outer border bevel */
	CSetColor (color_widget (0));
	XDrawRectangle (CDisplay, win, CGC, 0, 0, w - 1, h - 1);
    } else {
	render_bevel (win, 0, 0, w - 1, h - 1, 3, 1);	/*most outer border bevel */
    }
    bevel_background_color = COLOR_FLAT;
}

static void look_gtk_render_textinput_tidbits (CWidget * wdt, int isfocussed)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    bevel_background_color = COLOR_WHITE;
    if (isfocussed) {
	render_bevel (win, 1, 1, w - h - 2, h - 2, 2, 1);	/*most outer border bevel */
	CSetColor (color_widget (0));
	XDrawRectangle (CDisplay, win, CGC, 0, 0, w - h - 1, h - 1);
    } else {
	render_bevel (win, 0, 0, w - h - 1, h - 1, 3, 1);	/*most outer border bevel */
    }
    bevel_background_color = COLOR_FLAT;
    /* history button to the right */
    if (wdt->options & BUTTON_PRESSED) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, w - h + 2, 2, h - 4, h - 4);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 1);
    } else if (wdt->options & BUTTON_HIGHLIGHT) {
	bevel_background_color = color_widget (14);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 2);
	bevel_background_color = COLOR_FLAT;
    } else {
	CSetColor (COLOR_FLAT);
	CRectangle (win, w - h + 2, 2, h - 4, h - 4);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 0);
    }
}

extern struct focus_win focus_border;

static void look_gtk_render_focus_border (Window win)
{E_
    if (win == focus_border.top || win == focus_border.bottom || win == focus_border.left
	|| win == focus_border.right) {
	CSetColor (color_widget (0));
	CRectangle (win, 0, 0, focus_border.width + 2, focus_border.height + 2);
    }
}

static int look_gtk_get_extra_window_spacing (void)
{E_
    return 2;
}

static int look_gtk_get_focus_ring_size (void)
{E_
    return 1;
}

static unsigned long look_gtk_get_button_flat_color (void)
{E_
    return color_widget(12);
}

static int look_gtk_get_window_resize_bar_thickness (void)
{E_
    return 0;
}

static int look_gtk_get_switch_size (void)
{E_
    return FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2 + 4;
}

static int look_gtk_get_fielded_textbox_hscrollbar_width (void)
{E_
    return 12;
}

struct look look_gtk = {
    look_gtk_get_default_interwidget_spacing,
    look_gtk_menu_draw,
    look_gtk_get_menu_item_extents,
    look_gtk_render_menu_button,
    look_gtk_render_button,
    look_gtk_render_bar,
    look_gtk_render_raised_bevel,
    look_gtk_render_sunken_bevel,
    look_gtk_draw_hotkey_understroke,
    get_default_widget_font,
    look_gtk_render_text,
    look_gtk_render_window,
    look_gtk_render_scrollbar,
    look_gtk_get_scrollbar_size,
    look_gtk_init_scrollbar_icons,
    look_gtk_which_scrollbar_button,
    look_cool_scrollbar_handler,
    look_gtk_get_button_color,
    look_gtk_get_extra_window_spacing,
    look_cool_window_handler,
    look_gtk_get_focus_ring_size,
    look_gtk_get_button_flat_color,
    look_gtk_get_window_resize_bar_thickness,
    look_gtk_render_switch,
    look_gtk_get_switch_size,
    look_cool_draw_browser,
    look_cool_get_file_or_dir,
    look_cool_draw_file_list,
    look_cool_redraw_file_list,
    look_cool_get_file_list_line,
    look_cool_search_replace_dialog,
    look_gtk_edit_render_tidbits,
    look_cool_draw_exclam_cancel_button,
    look_cool_draw_tick_cancel_button,
    look_cool_draw_cross_cancel_button,
    look_cool_draw_tick_cancel_button,
    look_gtk_render_fielded_textbox_tidbits,
    look_gtk_render_textbox_tidbits,
    look_gtk_get_fielded_textbox_hscrollbar_width,
    look_gtk_render_textinput_tidbits,
    look_gtk_render_passwordinput_tidbits,
    look_gtk_render_focus_border,
};

