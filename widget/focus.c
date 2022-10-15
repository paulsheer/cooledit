/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* focus.c - records a history of focusses for reverting focus, also does focus cycling
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

/* #define FOCUS_DEBUG */


/* Focus stack: This stack remembers the focus history
so that when widgets are destroyed, the focus returns to
the most recent widget in the history. This is equivalent
to the revert_to in the XSetInputFocus() function,
however XSetInputFocus() has an effective history of
only one.

The commands to change focus are CFocus(CWidget *w)
*/

extern struct look *look;

void CRefreshSpot (void);

#define FOCUS_STACK_SIZE 128
static Window focus_stack[FOCUS_STACK_SIZE];
static int focus_sp = 0;
static Window current_focus = -1;
static Window current_ic_focus = -1;
static Window dnd_focus = -1;

void save_current_focus_for_dnd (Window new_focus)
{E_
    dnd_focus = current_focus;
    current_focus = new_focus;
}

void restore_current_focus_for_dnd (void)
{E_
    current_focus = dnd_focus;
}

void add_to_focus_stack (Window w)
{E_
    int i;
    i = focus_sp;
    while (i--)
	if (focus_stack[i] == w) {
	    focus_sp = i + 1;
	    return;
	}
    if (focus_sp >= FOCUS_STACK_SIZE) {
#ifdef FOCUS_DEBUG
/* NLS ? */
	printf ("add_to_focus_stack(): focus_sp overflow\n");
#endif
	return;
    }
    focus_stack[focus_sp++] = w;
#ifdef FOCUS_DEBUG
/* NLS ? */
    printf ("add_to_focus_stack(%x): focus_sp = %d\n", (unsigned int) w, focus_sp);
#endif
}

void focus_stack_remove_window (Window w)
{E_
    int i;
    i = focus_sp;
    while (i--)
	if (focus_stack[i] == w) {
	    focus_stack[i] = 0;
	    while (focus_sp && !focus_stack[focus_sp - 1])
		focus_sp--;
#ifdef FOCUS_DEBUG
/* NLS ? */
    printf ("focus_stack_remove_window(): focus_sp = %d\n", focus_sp);
#endif
	    return;
	}
}

Window CGetFocus(void)
{E_
    return current_focus;
}

Window CGetICFocus(void)
{E_
    return current_ic_focus;
}

void CFocusLast (void)
{E_
    Window w;
    if (!focus_sp)
	return;
    w = focus_stack[focus_sp - 1];
    if (w == current_focus)
	return;
    if (w)
	focus_window (w);
}

/*
   Each main window must record its last focussed window. This is so
   that when the window manager sets the focus to a main window. We
   can look up what widget within that window last had the focus, and
   move focus to that widget.
 */
static Window *get_last_focussed_in_main (Window main)
{E_
    static Window dummy;
    CWidget *w;
    w = CWidgetOfWindow (main);
    if (w)
	return &(w->last_child_focussed);
    dummy = 0;
    return &dummy;		/* instead of 'return 0;' to prevent segfaults */
}

/* {{{ Focus border creation and rendering */

/* this is four windows that surround the focussed window */
struct focus_win focus_border =
{
    0, 0, 0, 0, 0, 0, 0
};

Window get_focus_border_widget (void)
{E_
    return focus_border.current;
}

/* draw a focus window around wodget w */
void create_focus_border (CWidget * w, int border)
{E_
    int x, y;
    XSetWindowAttributes xswa;
    xswa.colormap = CColormap;
    xswa.bit_gravity = NorthWestGravity;
    if (border > 2)
	xswa.background_pixel = color_palette(17);	/* for drag and drop highlight borders */
    else
	xswa.background_pixel = COLOR_FLAT;
    if (w->parentid == CRoot) {
	xswa.override_redirect = 1;
    } else {
	xswa.override_redirect = 0;
    }

    focus_border.border = border;
    x = w->x;
    y = w->y;

    if (w->parentid == CRoot) {
	Window child;
	XTranslateCoordinates(CDisplay, w->winid, CRoot, 0, 0, &x, &y, &child);
    }

    focus_border.top =
	XCreateWindow (CDisplay, w->parentid,
		      x - WIDGET_FOCUS_RING, y - WIDGET_FOCUS_RING,
		     w->width + WIDGET_FOCUS_RING * 2, WIDGET_FOCUS_RING,
		 0, CDepth, InputOutput, CVisual,
	    CWOverrideRedirect | CWColormap | CWBackPixel | CWBitGravity,
			    &xswa);
    focus_border.bottom =
	XCreateWindow (CDisplay, w->parentid,
			     x - WIDGET_FOCUS_RING, y + w->height,
		     w->width + WIDGET_FOCUS_RING * 2, WIDGET_FOCUS_RING,
		 0, CDepth, InputOutput, CVisual,
	    CWOverrideRedirect | CWColormap | CWBackPixel | CWBitGravity,
			    &xswa);
    focus_border.left =
	XCreateWindow (CDisplay, w->parentid,
			     x - WIDGET_FOCUS_RING, y,
			     WIDGET_FOCUS_RING, w->height,
		 0, CDepth, InputOutput, CVisual,
	    CWOverrideRedirect | CWColormap | CWBackPixel | CWBitGravity,
			    &xswa);
    focus_border.right =
	XCreateWindow (CDisplay, w->parentid,
			     x + w->width, y,
			     WIDGET_FOCUS_RING, w->height,
		 0, CDepth, InputOutput, CVisual,
	    CWOverrideRedirect | CWColormap | CWBackPixel | CWBitGravity,
			    &xswa);
    focus_border.current = w->winid;
    focus_border.width = w->width;
    focus_border.height = w->height;
    XSelectInput (CDisplay, focus_border.top, ExposureMask);
    XSelectInput (CDisplay, focus_border.bottom, ExposureMask);
    XSelectInput (CDisplay, focus_border.left, ExposureMask);
    XSelectInput (CDisplay, focus_border.right, ExposureMask);
    XMapWindow (CDisplay, focus_border.top);
    XMapWindow (CDisplay, focus_border.bottom);
    XMapWindow (CDisplay, focus_border.left);
    XMapWindow (CDisplay, focus_border.right);
}

void destroy_focus_border (void)
{E_
    if (!focus_border.top)
	return;
    XDestroyWindow (CDisplay, focus_border.top);
    XDestroyWindow (CDisplay, focus_border.bottom);
    XDestroyWindow (CDisplay, focus_border.left);
    XDestroyWindow (CDisplay, focus_border.right);
    memset (&focus_border, 0, sizeof (focus_border));
}

int window_of_focus_border (Window win)
{E_
    if (!focus_border.top)
	return 0;
    if (win == focus_border.top)
	return 1;
    if (win == focus_border.bottom)
	return 1;
    if (win == focus_border.left)
	return 1;
    if (win == focus_border.right)
	return 1;
    return 0;
}

void render_focus_border (Window win)
{E_
    (*look->render_focus_border) (win);
}

static void set_ic_focus (CWidget * w)
{E_
#ifdef USE_XIM
    XIC ic;
    if (w->mainid) {
	ic = (CWidgetOfWindow (w->mainid))->input_context;
	current_ic_focus = w->mainid;
    } else {
	ic = w->input_context;
	current_ic_focus = 0;
    }
    if (ic) {
	XSetICFocus (ic);
#ifdef FOCUS_DEBUG
printf("CRoot=%lu parentid=%lu\n", (unsigned long) CRoot, (unsigned long) w->parentid);
#endif
	cim_send_spot (w->mainid);
    }
#endif
}

/* }}} Focus border creation and rendering */

/* This is called when the app (and not the WM) wants to change focus. */
static void focus_widget (CWidget * w)
{E_
    CWidget *old;

    CRefreshSpot ();

    if (current_focus == w->winid) {
#ifdef FOCUS_DEBUG
	printf ("current_focus == w->winid, returning\n");
#endif
	return;
    }

    destroy_focus_border ();
    if ((w->options & WIDGET_TAKES_FOCUS_RING))
	create_focus_border (w, 1);

    old = CWidgetOfWindow (current_focus);

    current_focus = w->winid;	/* notify library of new focus */

    CSendMessage (old, FocusOut);	/* unfocus the current focus */

/* focus on the widgets main window if it has changed */
    if (old) {
	if (old->mainid != w->mainid)
	    goto set_main;
    } else {
      set_main:
/* set that main windows 'last_child_focussed' */
 	XSetInputFocus (CDisplay, w->mainid, RevertToPointerRoot, CurrentTime);
#ifdef FOCUS_DEBUG
	printf("XSetInputFocus\n");
#endif
	set_ic_focus (w);
    }
    *(get_last_focussed_in_main (w->mainid)) = w->winid;
#ifdef FOCUS_DEBUG
    printf ("setting last_focus of %s = %s\n", (char *) CWidgetOfWindow (w->mainid), w->ident);
#endif

    add_to_focus_stack (w->winid);	/* record focus history */

/* focus the new widget */
    CSendMessage (w, FocusIn);
}

static void focus_main (Window main, int type)
{E_
    CWidget *w;

    CRefreshSpot ();

    if (type == FocusOut) {
	w = CWidgetOfWindow (current_focus);
#ifdef FOCUS_DEBUG
	printf ("focus_main(Out): %s\n", w->ident);
#endif
	current_focus = -1;	/* inform the library that nothing is focussed */
	CSendMessage (w, FocusOut);
	destroy_focus_border ();
    } else {			/* type == FocusIn */
/* find the widget within this main that last had the focus: */
	current_focus = *(get_last_focussed_in_main (main));

	w = CWidgetOfWindow (current_focus);
#ifdef FOCUS_DEBUG
	printf ("focus_main(In): %s\n", w->ident);
#endif
	if (w) {
	    add_to_focus_stack (w->winid);	/* record focus history */
	    CSendMessage (w, FocusIn);	/* focus the new widget */
	    if ((w->options & WIDGET_TAKES_FOCUS_RING))
		create_focus_border (w, 2);
	    set_ic_focus (w);
	}
    }
}

/* 
   This is called from CNextEvent when the WM sends
   focus to a new different main window.
 */
void process_external_focus (Window win, int type)
{E_
    CWidget *w;
#ifdef FOCUS_DEBUG
    printf ("Entering process_external_focus()\n");
#endif
    w = CWidgetOfWindow (win);
    if (!w) {
#ifdef FOCUS_DEBUG
	printf ("Leaving process_external_focus() - not our window\n");
#endif
	return;
    }
#ifdef FOCUS_DEBUG
    printf ("widget = %s\n", w->ident);
#endif
    if (w->parentid == CRoot) {	/* should always be true since only 
				   main windows have FocusChangeMask */
	focus_main (w->winid, type);
#ifdef FOCUS_DEBUG
	printf ("Leaving process_external_focus() - main window\n");
#endif
	return;
    } else {
#ifdef FOCUS_DEBUG
	printf ("Leaving process_external_focus() - not main window\n");
#endif
    }
}

void focus_window (Window win)
{E_
    CWidget *w;
    w = CWidgetOfWindow (win);
    if (w)
	CFocusNormal (w);
}

/* CFocus() is a macro for */
void CFocusNormal (CWidget * w)
{E_
    if (!w)
	return;
    if (!w->takes_focus)
	return;
    if (w->mapped & WINDOW_MAPPED) {
	focus_widget (w);
    } else {
	w->mapped |= WINDOW_FOCUS_WHEN_MAPPED;
    }
}

extern int option_never_raise_wm_windows;

int CTryFocus (CWidget * w, int raise_wm_window)
{E_
    if (!option_never_raise_wm_windows) {
	CFocus (w);
	if (raise_wm_window)
	    CRaiseWMWindow (w->ident);
	return 1;
    } else {
/* focus only if it does not involve switching focus of a WM window: */
	CWidget *v;
	Window *win;
	v = CWidgetOfWindow (CGetFocus ());
	if (v)
	    if (v->mainid == w->mainid) {
		CFocus (w);
		return 1;
	    }
#ifdef FOCUS_DEBUG
	printf ("setting last_focussed to %s\n", w->ident);
#endif
	win = get_last_focussed_in_main (w->mainid);
	if (*win)
	    add_to_focus_stack (*win);
	add_to_focus_stack ((*win = w->winid));
    }
    return 0;
}

void CFocusDebug (CWidget *w, int line, char *file)
{E_
/* NLS ? */
    printf ("CFocus(%x): %s:%d (ident = %s)\n", (unsigned int) w->winid, file, line, w->ident);
    CFocusNormal (w);
}


/* get next sibling of w that has takes_focus set (i.e. that takes user input of any sort) */
CWidget *CNextFocus (CWidget * w)
{E_
    int i, j;
    if (!w)
	return 0;
    i = j = find_next_child_of (w->parentid, w->winid);
    for (;;) {
	if (!i) {
	    i = find_first_child_of (w->parentid);
	    if (!i)
		return 0;
	}
	if (CIndex (i)->takes_focus && !CIndex(i)->disabled)
	    return CIndex (i);
	w = CIndex (i);
	i = find_next_child_of (w->parentid, w->winid);
	if (i == j) /* done a round trip */
	    return 0;
    }
}

/* previous sibling of same */
CWidget *CPreviousFocus (CWidget * w)
{E_
    int i, j;
    i = j = find_previous_child_of (w->parentid, w->winid);
    for (;;) {
	if (!i) {
	    i = find_last_child_of (w->parentid);
	    if (!i)
		return 0;
	}
	if (CIndex (i)->takes_focus && !CIndex(i)->disabled)
	    return CIndex (i);
	w = CIndex (i);
	i = find_previous_child_of (w->parentid, w->winid);
	if (i == j) /* done a round trip */
	    return 0;
    }
}

/* first child of widget that takes focus (eg w is a window and
    a button in the window is returned) */
CWidget *CChildFocus (CWidget * w)
{E_
    int j, i = find_first_child_of (w->winid);
    if(!i)
	return 0;
    w = CIndex (i);
    if(w->takes_focus)
	return w;
    j = i = find_next_child_of (w->parentid, w->winid);
    for (;;) {
	if (!i) {
	    i = find_first_child_of (w->parentid);
	    if (!i)
		return 0;
	}
	if (CIndex (i)->takes_focus)
	    return CIndex (i);
	w = CIndex (i);
	i = find_next_child_of (w->parentid, w->winid);
	if (i == j) /* done a round trip */
	    return 0;
    }
}

/* search for two generations down for the first descendent that is a widget.
   If it does not take focus, then its first child is focussed.
   If it has no children, the next descendent is searched for. */
CWidget *CFindFirstDescendent (Window win)
{E_
    int i, j;

    i = find_first_child_of (win);
    if (i) {			/* is it a child ? */
	if (CIndex (i)->takes_focus && !CIndex (i)->disabled) {
	    return (CIndex (i));
	} else {
	    CWidget *w;
	    w = CChildFocus (CIndex (i));
	    if (w)
		return w;
	}
    } else {			/* not a child */
	Window root, parent, *children = 0;
	unsigned int nchildren = 0;
	if (!win)
	    return 0;
	XQueryTree (CDisplay, win, &root, &parent, &children, &nchildren);
	if (!nchildren) {
	    if (children)
		XFree (children);
	    return 0;
	}
	for (j = 0; j < nchildren; j++)
	    if ((i = find_first_child_of (children[j]))) {	/* is it a grandchild ? */
		if (CIndex (i)->takes_focus && !CIndex (i)->disabled) {
		    XFree (children);
		    return (CIndex (i));
		} else {
		    CWidget *w;
		    w = CChildFocus (CIndex (i));
		    if (w) {
			XFree (children);
			return w;
		    }
		}
	    }
	XFree (children);
    }
    return 0;			/* not a grandchild */
}

