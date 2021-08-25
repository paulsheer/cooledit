/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* menu.c - proper 3D menus
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

char current_pulled_button[33] = "";

int menu_grabbed = 0;

int eh_menu (CWidget * w, XEvent * xevent, CEvent * cwevent);
void CMenuSelectionDialog (CWidget * button);

static void render_menu_button (CWidget * wdt);

static int get_dropped (CWidget **r, CWidget *w)
{E_
    *r = 0;
    if (!w->droppedmenu[0])
	return 0;
    *r = CIdent (w->droppedmenu);
    if (!*r)
	return 0;
    return 1;
}

void destroy_menu (CWidget * w)
{E_
    int i;
    if (!w)
	return;
    if (!w->menu)
	return;
    for (i = 0; i < w->numlines; i++)
	if (w->menu[i].text)
	    free (w->menu[i].text);
    free (w->menu);
}

/* 
   Gets y1 and y2 of the entry of a menu (i.e. a menu item). x1, y1,
   x2, y2 would describe the rectangle that encloses that entry,
   relative to the top left corner of the pulled-down menu window.
 */
void get_menu_item_extents (int n, int j, struct menu_item m[], int *border, int *relief, int *y1, int *y2)
{E_
    (*look->get_menu_item_extents) (n, j, m, border, relief, y1, y2);
}

/*
   Returns 0-(n-1) where n is the number of entries in that menu. Returns
   which menu item the pointer is in if (x,y) is the pointer pos. Returns
   -1 if out of bounds or between menu items.
 */
int whereis_pointer (int x, int y, int w, int n, struct menu_item m[])
{E_
    int i, y1, y2, border, relief;
    CPushFont ("widget");
    for (i = 0; i < n; i++) {
	if (!m[i].text[2])
	    continue;
	get_menu_item_extents (n, i, m, &border, &relief, &y1, &y2);
	if (y >= y1) {
	    if (y < y2 && x >= border && x < w - border)
		break;
	} else {
            i = -1;
	    break;
        }
    }
#warning backport this fix
    if (i >= n)
        i = n - 1;
    CPopFont ();
    return i;
}

int find_menu_hotkey (struct menu_item m[], int this, int num)
{E_
    unsigned char used_keys[256];
    int n = 0, j;
    if (!num)
	return 0;
    for (j = 0; j < num; j++)
	if (m[j].hot_key && j != this)
	    used_keys[n++] = my_lower_case (m[j].hot_key);
    return find_letter_at_word_start ((unsigned char *) m[this].text + 1, used_keys, n);
}

void menu_draw (Window win, int w, int h, struct menu_item m[], int n, int light)
{E_
    (*look->menu_draw) (win, w, h, m, n, light);
}

void render_menu (CWidget * w)
{E_
    CWidget *drop;
    int n, border, relief, y1, y2, i;
    unsigned new_width, new_height;
    if (!w)
	return;
    CPushFont ("widget", 0);
    n = w->numlines;
    get_menu_item_extents (n, n - 1, w->menu, &border, &relief, &y1, &y2);
    new_height = y2 + border;
    new_width = 0;
    for (i = 0; i < n; i++) {
	int t, v;
        v = strcspn(w->menu[i].text, "\t");
	t = CImageTextWidth (w->menu[i].text, v);       /* pixel width of part before the Tab */
	t += CImageStringWidth ("W");                   /* pixel width of some typographic space */
        if (w->menu[i].text[v])
            v++;
	t += CImageStringWidth (w->menu[i].text + v);   /* pixel width of part after the Tab */
	if (new_width < t)
	    new_width = t;
    }
    new_width += (border + relief) * 2;
    if (w->width != new_width || w->height != new_height) {
	w->width = new_width;
	w->height = new_height;
	XResizeWindow (CDisplay, w->winid, w->width, w->height);
    }
#if 0
    if (w->y + w->height > HeightOfScreen (DefaultScreenOfDisplay (CDisplay))) {
	int y;
	y = HeightOfScreen (DefaultScreenOfDisplay (CDisplay)) - w->height;
	if (y < 0)
	    y = 0;
	XMoveWindow (CDisplay, w->winid, w->x, y);
    }
#endif
#define BOUND_LIMITS 50
    get_menu_item_extents (n, w->current, w->menu, &border, &relief, &y1, &y2);
    if (w->current >= 0) {
	if (y2 + w->y + BOUND_LIMITS >= HeightOfScreen (DefaultScreenOfDisplay (CDisplay)))
	    CSetWidgetPosition (w->ident, w->x, HeightOfScreen (DefaultScreenOfDisplay (CDisplay)) - y2 - BOUND_LIMITS);
	if (y1 + w->y < BOUND_LIMITS)
	    CSetWidgetPosition (w->ident, w->x, BOUND_LIMITS - y1);
    }
    if (get_dropped (&drop, w))
        drop->current = w->current;
    menu_draw (w->winid, w->width, w->height, w->menu, w->numlines, w->current);
    CPopFont ();
    return;
}

/* gets a windows position relative to the origin if some ancestor windows,
   window can be of a widget or not */
void CGetWindowPosition (Window win, Window ancestor, int *x_return, int *y_return)
{E_
    CWidget *w = (CWidget *) 1;
    int x = 0, y = 0;
    Window root, parent, *children;
    unsigned int nchildren, width, height, bd, depth;
    *x_return = *y_return = 0;
    if (win == ancestor)
	return;
    for (;;) {
	if (w)
	    w = CWidgetOfWindow (win);
	if (w)
	    if (w->parentid == CRoot)
		w = 0;
	if (w) {
	    parent = w->parentid;
	    x = w->x;
	    y = w->y;
	} else {
	    if (!XQueryTree (CDisplay, win, &root, &parent, &children, &nchildren))
		return;
	    if (children)
		XFree ((char *) children);
	    XGetGeometry (CDisplay, win, &root, &x, &y, &width, &height, &bd, &depth);
	}
	*x_return += x;
	*y_return += y;
	if (parent == ancestor || parent == CRoot)
	    break;
	win = parent;
    }
}

void focus_stack_remove_window (Window w);

void pull_up (CWidget * button)
{E_
    if (!button)
	return;
    if (button->kind != C_MENU_BUTTON_WIDGET)
	return;
    if (button->droppedmenu[0]) {
	current_pulled_button[0] = '\0';
	CDestroyWidget (button->droppedmenu);
	button->droppedmenu[0] = '\0';
    }
    focus_stack_remove_window (button->winid);
    render_menu_button (button);
}

static CWidget *last_menu = 0;

void CSetLastMenu (CWidget *button)
{E_
    last_menu = button;
}

CWidget *CGetLastMenu (void)
{E_
    return last_menu;
}

void menu_hand_cursor (Window win);

static CWidget *pull_down (CWidget * button) /* must create a new widget */
{E_
    CWidget *menu;
    CWidget *sib;
    int width, height, n;
    int x, y;

    if (button->droppedmenu[0])
	return 0;

    sib = CGetLastMenu ();
    if (sib)
	if (strcmp (button->ident, sib->ident))
	    pull_up (sib);			/* pull up last menu if different */

    sib = button;
    while((sib = CNextFocus (sib)) != button)	/* pull up any other sibling menus */
	pull_up (sib);

    CSetLastMenu (button);

    n = button->numlines;

    CGetWindowPosition (button->winid, CRoot, &x, &y);

    height = 2;	/* don't know what these are yet */
    width = 2;

    x += button->firstcolumn;

    menu = CSetupWidget (catstrs (button->ident, ".pull", NULL), CRoot, x,
	   y + button->height, width, height, C_MENU_WIDGET, INPUT_KEY, COLOR_FLAT, 0);
    menu->options |= (button->options & MENU_AUTO_PULL_UP);

    menu_hand_cursor (menu->winid);

/* no destroy 'cos gets ->menu gets destroyed by other menu-button-widget */
    menu->numlines = n;
    menu->menu = button->menu;
    menu->eh = eh_menu;
    strcpy (menu->droppedmenu, button->ident);
    strcpy (button->droppedmenu, menu->ident);
    strcpy (current_pulled_button, button->ident);
    render_menu_button (button);
    return menu;
}

void CPullDown (CWidget * button)
{E_
    pull_down (button);
}

CWidget *get_pulled_menu (void)
{E_
    return CIdent (current_pulled_button);
}

void CPullUp (CWidget * button)
{E_
    pull_up (button);
}

int execute_item (CWidget * menu, int item)
{E_
    CWidget *drop = 0;
    int r = 0;
    char ident[33];
    strcpy (ident, menu->ident);
    if (get_dropped (&drop, menu))
        drop->current = item;
    XUngrabPointer (CDisplay, CurrentTime);
    XUnmapWindow (CDisplay, menu->winid);
    if (item >= 0 && item < menu->numlines)
	if (menu->menu[item].call_back) {
            if (drop)
                drop->current = item;
            current_pulled_button[0] = '\0';
	    (*(menu->menu[item].call_back)) (menu->menu[item].data);
	    r = 1;
	}
/* may not exist so do menu ?= CIdent(ident) */
    if ((menu = CIdent (ident)) && get_dropped (&drop, menu))
        pull_up (drop);
    CFocusLast ();
    return r;
}

static void render_menu_button (CWidget * wdt)
{E_
    (*look->render_menu_button) (wdt);
}


int is_focus_change_key (KeySym k, int command);
int is_focus_prev_key (KeySym k, int command, unsigned int state);

int eh_menubutton (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    int c;
    CWidget *f, *drop;
    switch (xevent->type) {
    case FocusOut:
    case FocusIn:
	render_menu_button (w);
	CExposeWindowArea (w->parentid, 0, w->x - WIDGET_FOCUS_RING, w->y - WIDGET_FOCUS_RING, w->width + WIDGET_FOCUS_RING * 2, w->height + WIDGET_FOCUS_RING * 2);
	break;
    case KeyPress:
	c = CKeySym (xevent);
	if (!w->droppedmenu[0]) {
	    if (c == XK_space || c == XK_Return || c == XK_KP_Enter || cwevent->command == CK_Down) {
		CMenuSelectionDialog (w);
		return 1;
	    }
	}
	if (c == XK_Escape) {
	    pull_up (w);
	    CFocusLast ();
	    return 1;
	}

/* this here is very messy with a lot of defensive programming :-(  */
	if (cwevent->command == CK_Up && get_dropped (&drop, w)) {
	    if (drop->numlines < 1)
		return 1;
	    if (drop->current == -1)
		drop->current = 0;
	    do {
		drop->current = (drop->current + drop->numlines - 1) % drop->numlines;
	    } while (drop->menu[drop->current].text[2] == 0);
	    render_menu (drop);
	    return 1;
	}
	if (cwevent->command == CK_Down && get_dropped (&drop, w)) {
	    if (drop->numlines < 1)
		return 1;
	    do {
		drop->current = (drop->current + 1) % drop->numlines;
	    } while (drop->menu[drop->current].text[2] == 0);
	    render_menu (drop);
	    return 1;
	}
	if (is_focus_prev_key (c, cwevent->command, xevent->xkey.state)) {
	    f = CPreviousFocus (w);
	    while (f->kind != C_MENU_BUTTON_WIDGET && (unsigned long) f != (unsigned long) w)
		f = CPreviousFocus (f);
	    if (f) {
		CFocus (f);
		if (get_dropped (&drop, w))
		    CMenuSelectionDialog (f);
	    }
	    return 1;
	}
	if (is_focus_change_key (c, cwevent->command)) {
	    f = CNextFocus (w);
	    while (f->kind != C_MENU_BUTTON_WIDGET && (unsigned long) f != (unsigned long) w)
		f = CNextFocus (f);
	    if (f) {
		CFocus (f);
		if (get_dropped (&drop, w))
		    CMenuSelectionDialog (f);
	    }
	    return 1;
	}
	if (c && get_dropped (&drop, w)) {
	    int i;
	    if (c == XK_Return || c == XK_KP_Enter || c == XK_space) {
		return execute_item (drop, drop->current);
	    } else {
		for (i = 0; i < drop->numlines; i++)
		    if (match_hotkey (c, drop->menu[i].hot_key))
			return execute_item (drop, i);
	    }
	}
	if (cwevent->command != CK_Up && cwevent->command != CK_Down)
	    return 0;
    case ButtonPress:
	if (xevent->type == ButtonPress) {
	    w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	    w->options |= BUTTON_PRESSED;
	}
	render_menu_button (w);
	if (!w->droppedmenu[0])
	    CMenuSelectionDialog (w);
	return 1;
	break;
    case ButtonRelease:
	w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	w->options |= BUTTON_HIGHLIGHT;
	render_menu_button (w);
	return 1;
    case MotionNotify:
	if (!w->droppedmenu[0] && menu_grabbed) {
	    pull_down (w);
	    CFocus (w);
	}
	return 1;
    case EnterNotify:
	w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	w->options |= BUTTON_HIGHLIGHT;
	render_menu_button (w);
	break;
    case Expose:
	if (xevent->xexpose.count)
	    break;
    case LeaveNotify:
	w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	render_menu_button (w);
	break;
    }
    return 0;
}



int eh_menu (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    CWidget *drop;
    static Window win = 0;
    static int current = -30000;
    switch (xevent->type) {
    case MotionNotify:
	w->current = whereis_pointer (xevent->xmotion.x, xevent->xmotion.y, w->width, w->numlines, w->menu);
	if (w->current == current && w->winid == win)
	    break;
	current = w->current;
	win = w->winid;
	render_menu (w);
	break;
    case ButtonRelease:
	return execute_item (w, whereis_pointer (xevent->xmotion.x, xevent->xmotion.y, w->width, w->numlines, w->menu));
    case ButtonPress:
	w->current = whereis_pointer (xevent->xmotion.x, xevent->xmotion.y, w->width, w->numlines, w->menu);
	render_menu (w);
	break;
    case Expose:
	if (xevent->xexpose.count)
	    break;
    case LeaveNotify:
        if (get_dropped (&drop, w))
	    current = w->current = drop->current;
	render_menu (w);
	break;
    }
    return 0;
}

CWidget *CDrawMenuButton (const char *ident, Window parent, Window focus_return,
   int x, int y, int width, int height, int num_items, const char *label,...)
{E_
    va_list ap;
    CWidget *wdt;
    struct menu_item *m;
    int i;
    int w, h;

    CPushFont ("widget");

    if (width == AUTO_WIDTH || height == AUTO_HEIGHT)
	CTextSize (&w, &h, label);
    if (width == AUTO_WIDTH)
	width = w + 4 + BUTTON_RELIEF * 2;
    if (height == AUTO_HEIGHT)
	height = h + 4 + BUTTON_RELIEF * 2;

    wdt = CSetupWidget (ident, parent, x, y,
			width, height, C_MENU_BUTTON_WIDGET, INPUT_KEY | OwnerGrabButtonMask, COLOR_FLAT, 1);
    wdt->options |= MENU_AUTO_PULL_UP;		/* default */

    set_hint_pos (x + width, y + height + WIDGET_SPACING);
    wdt->label = (char *) strdup (label);
    wdt->hotkey = find_hotkey (wdt);
    wdt->options |= WIDGET_HOTKEY_ACTIVATES;

    m = CMalloc ((num_items ? num_items : 1) * sizeof (struct menu_item));

    va_start (ap, label);
    for (i = 0; i < num_items; i++) {
	char *text;
	text = va_arg (ap, char *);
	text = text ? text : "";
	m[i].text = (char *) strdup (catstrs (" ", text, " ", NULL));
	m[i].hot_key = va_arg (ap, int);
	m[i].call_back = va_arg (ap, callfn);
	m[i].data = va_arg (ap, unsigned long);
    }
    va_end (ap);

    wdt->destroy = destroy_menu;
    wdt->numlines = num_items;
    wdt->menu = m;
    wdt->eh = eh_menubutton;

    CPopFont ();
    return wdt;
}

void insert_menu_item (CWidget * w, int i, const char *text, int hot_key, callfn call_back, unsigned long data)
{E_
    struct menu_item *m;
    CWidget *drop;

    m = CMalloc ((w->numlines + 1) * sizeof (struct menu_item));
    memcpy (m, w->menu, i * sizeof (struct menu_item));
    memcpy (m + i + 1, w->menu + i, (w->numlines - i) * sizeof (struct menu_item));
    free (w->menu);
    w->menu = m;
    m[i].text = (char *) strdup (catstrs (" ", text, " ", NULL));
    m[i].hot_key = hot_key;
    m[i].call_back = call_back;
    m[i].data = data;

    w->numlines++;

    if (get_dropped (&drop, w)) {
	drop->menu = m;
	drop->numlines = w->numlines;
	drop->current = w->current;
	render_menu (drop);
    }
}

void CAddMenuItem (const char *ident, const char *text, int hot_key, callfn call_back, unsigned long data)
{E_
    CWidget *w;
    w = CIdent (ident);
    if (!w) {
	CErrorDialog (0, 0, 0, _ (" Add Menu Item "), " %s: %s ", _ ("No such menu"), ident);
	return;
    }
    insert_menu_item (w, w->numlines, text, hot_key, call_back, data);
}

void CInsertMenuItem (const char *ident, const char *after, const char *text, int hot_key, callfn call_back, unsigned long data)
{E_
    int i;
    CWidget *w;
    w = CIdent (ident);
    if (!w) {
	CErrorDialog (0, 0, 0, _ (" Insert Menu Item "), " %s: %s ", _ ("No such menu"), ident);
	return;
    }
    i = CHasMenuItem (ident, after);
    if (i < 0) {
	CErrorDialog (0, 0, 0, _ (" Insert Menu Item "), " %s: %s ", _ ("No such item"), after);
	return;
    }
    insert_menu_item (w, i, text, hot_key, call_back, data);
}

void CInsertMenuItemAfter (const char *ident, const char *after, const char *text, int hot_key, callfn call_back, unsigned long data)
{E_
    int i;
    CWidget *w;
    w = CIdent (ident);
    if (!w) {
	CErrorDialog (0, 0, 0, _ (" Insert Menu Item "), " %s: %s ", _ ("No such menu"), ident);
	return;
    }
    i = CHasMenuItem (ident, after);
    if (i < 0) {
	CErrorDialog (0, 0, 0, _ (" Insert Menu Item "), " %s: %s ", _ ("No such item"), after);
	return;
    }
    insert_menu_item (w, i + 1, text, hot_key, call_back, data);
}

static void remove_item (CWidget * w, int i)
{E_
    CWidget *drop;
    if (!w)
	return;
    if (i >= w->numlines || i < 0)
	return;
    if (w->menu[i].text)
	free (w->menu[i].text);
    w->numlines--;
    Cmemmove (&w->menu[i], &w->menu[i + 1], (w->numlines - i) * sizeof (struct menu_item));
    if (w->current == i)
	w->current = -1;
    else if (w->current > i)
	w->current--;
    if (get_dropped (&drop, w)) {
	drop->numlines = w->numlines;
	drop->current = w->current;
    }
}

void CRemoveMenuItemNumber (const char *ident, int i)
{E_
    CWidget *w;
    w = CIdent (ident);
    remove_item (w, i);
}

/*
   Starts from the bottom of the menu and searches for the first
   menu item containing text (strstr != NULL), and returns the
   integer.
 */
int CHasMenuItem (const char *ident, const char *text)
{E_
    CWidget *w;
    int i;
    w = CIdent (ident);
    if (!w)
	return -1;
    if (w->numlines)
	for (i = w->numlines - 1; i >= 0; i--)
	    if (strstr (w->menu[i].text, text) || !*text)
		return i;
    return -1;
}

/*
   Starts from the bottom of the menu and searches for the first
   menu item containing text (strstr != NULL), and deletes it.
 */
void CRemoveMenuItem (const char *ident, const char *text)
{E_
    remove_item (CIdent (ident), CHasMenuItem (ident, text));
}

void CReplaceMenuItem (const char *ident, const char *old_text, const char *new_text, int hot_key, callfn call_back, unsigned long data)
{E_
    struct menu_item *m;
    CWidget *w, *drop;
    int i;

    w = CIdent (ident);
    if (!w) {
	CErrorDialog (0, 0, 0, _ (" Replace Menu Item "), " %s: %s ", _ ("No such menu"), ident);
	return;
    }
    i = CHasMenuItem (ident, old_text);
    if (i < 0) {
	CErrorDialog (0, 0, 0, _ (" Replace Menu Item "), " %s: %s ", _ ("No such item"), old_text);
	return;
    }
    m = w->menu;
    free (m[i].text);
    m[i].text = (char *) strdup (catstrs (" ", new_text, " ", NULL));
    m[i].hot_key = hot_key;
    m[i].call_back = call_back;
    m[i].data = data;

    if (get_dropped (&drop, w))
	render_menu (drop);
}

void CMenuSelectionDialog (CWidget * button)
{E_
    CEvent cwevent;
    XEvent xevent;

/* sanity check */
    if (!button)
	return;

/* drop the menu and focus on the button */
    pull_down (button);
    CFocus (button);

/* we are already inside this function */
    if (menu_grabbed)
	return;
    menu_grabbed = 1;

/* grab the pointer - we want events even when pointer is over other apps windows */
    XGrabPointer
	(CDisplay, button->winid, True,
	 ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask |
	 EnterWindowMask | LeaveWindowMask | OwnerGrabButtonMask, GrabModeAsync,
	 GrabModeAsync, None, CGetCursorID (CURSOR_MENU), CurrentTime);

/* current_pulled_button could go to zero if some handler does a pull_up of the menu */
    while (current_pulled_button[0]) {
	CDisableXIM();      /* to get raw key */
	CNextEvent (&xevent, &cwevent);
	CEnableXIM();
/* terminate the loop if we have a button press/release as the user expects */
	if (xevent.type == ButtonRelease || xevent.type == ButtonPress) {
	    CWidget *w;
	    w = CWidgetOfWindow (xevent.xbutton.window);
	    if (!w)
		break;
	    if (w->kind != C_MENU_BUTTON_WIDGET && w->kind != C_MENU_WIDGET)
		break;
	    if (xevent.xbutton.x >= w->width || xevent.xbutton.x < 0 ||
		xevent.xbutton.y >= w->height || xevent.xbutton.y < 0)
		break;
	}
    }

/* check if any menu up */
    if (current_pulled_button[0]) {
	pull_up (CIdent (current_pulled_button));
	current_pulled_button[0] = '\0';
    }

/* ungrab and return focus */
    menu_grabbed = 0;
    XUngrabPointer (CDisplay, CurrentTime);
    CFocusLast ();
}


