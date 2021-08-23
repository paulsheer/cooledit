/* scrollbar.c - scrollbar widget
   Copyright (C) 1996-2018 Paul Sheer
 */



#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <X11/Xatom.h>
#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"
#include "edit.h"
#include "editcmddef.h"

#include "coollocal.h"
#include "mousemark.h"


extern struct look *look;

CWidget *CDrawVerticalScrollbar (const char *identifier, Window parent, int x, int y,
				 int length, int width, int pos, int prop)
{
    CWidget *w;
    if (width == AUTO_WIDTH || width == AUTO_HEIGHT)
	width = (*look->get_scrollbar_size) (C_VERTSCROLL_WIDGET);
    w = CSetupWidget (identifier, parent, x, y,
	width, length, C_VERTSCROLL_WIDGET, INPUT_MOTION, COLOR_FLAT, 0);
    w->firstline = pos;
    w->numlines = prop;
    set_hint_pos (x + width + WIDGET_SPACING, y + length + WIDGET_SPACING);
    (*look->init_scrollbar_icons) (w);
    return w;
}

CWidget *CDrawHorizontalScrollbar (const char *identifier, Window parent, int x, int y,
			     int length, int width, int pos, int prop)
{
    CWidget *w;
    if (width == AUTO_WIDTH || width == AUTO_HEIGHT)
	width = (*look->get_scrollbar_size) (C_HORISCROLL_WIDGET);
    w = CSetupWidget (identifier, parent, x, y,
		      length, width, C_HORISCROLL_WIDGET,
		      ExposureMask | ButtonPressMask |
	       ButtonReleaseMask | ButtonMotionMask | PointerMotionMask |
		      EnterWindowMask | LeaveWindowMask, COLOR_FLAT, 0);
    w->firstline = pos;
    w->numlines = prop;
    set_hint_pos (x + length + WIDGET_SPACING, y + width + WIDGET_SPACING);
    (*look->init_scrollbar_icons) (w);
    return w;
}

void CSetScrollbarCallback (const char *scrollbar, const char *wdt,
			    void (*link_to) (CWidget *,
				      CWidget *, XEvent *, CEvent *, int))
{
    CWidget *s, *w;
    s = CIdent (scrollbar);
    w = CIdent (wdt);
    if (!s || !w)
	return;
    s->vert_scrollbar = w;
    s->scroll_bar_link = link_to;
}

void render_scrollbar (CWidget * wdt)
{
    (look->render_scrollbar) (wdt);
}

int inbounds (int x, int y, int x1, int y1, int x2, int y2)
{
    if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
	return 1;
    else
	return 0;
}

int eh_scrollbar (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    return (*look->scrollbar_handler) (w, xevent, cwevent);
}


