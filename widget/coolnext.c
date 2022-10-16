/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* coolnext.c - CNextEvent function and support, and various event
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

#include "drawings.h"
#include "edit.h"
#include "editcmddef.h"
#include "widget3d.h"




static int last_region = 0;

int run_callbacks (CWidget * w, XEvent * xevent, CEvent * cwevent);


int (*global_alarm_callback[33]) (CWidget *, XEvent *, CEvent *) = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
extern Pixmap Cswitchon, Cswitchoff;
#ifdef HAVE_DND
extern Atom DndProtocol, OldDndProtocol;
#endif

void add_to_focus_stack (Window w);
void selection_clear (void); /* from editwidget.c */

int option_mouse_double_click = 300;
int option_middle_button_pastes = 1;

/* converts button presses from buttons 2 through 5 to button presses from 2 only, also gets double clicks */
void resolve_button (XEvent * xevent, CEvent * cwevent)
{E_
    static Time thyme_press = 0, thyme_release = 0;
    static Window window = 0;
    static int x, y;
    cwevent->state = xevent->xbutton.state;
    if (cwevent->state & (Button2Mask | Button3Mask /*| Button4Mask | Button5Mask */ ))
	cwevent->state |= Button2Mask;
    switch (xevent->type) {
    case ButtonRepeat:
    case ButtonRelease:
    case ButtonPress:
	cwevent->button = xevent->xbutton.button;
	if (cwevent->button == Button4 || cwevent->button == Button5) {
	    /* ahaack: wheel mouse mapped as button 4 and 5 */
	    return;
	}
	if (cwevent->button == Button2 || cwevent->button == Button3
	    || cwevent->button == Button4 || cwevent->button == Button5) {
	    cwevent->button = Button2;
	}
	cwevent->x = xevent->xbutton.x;
	cwevent->y = xevent->xbutton.y;
	if (xevent->type != ButtonRepeat) {
	    if (window == xevent->xany.window)
		if (abs (x - cwevent->x) < 4 && abs (y - cwevent->y) < 4) {
		    if (labs ((long) xevent->xmotion.time - (long) thyme_press) < option_mouse_double_click &&
			xevent->type == ButtonPress)
			thyme_press = cwevent->double_click = 1;
		    if (labs ((long) xevent->xmotion.time - (long) thyme_release) < option_mouse_double_click &&
			xevent->type == ButtonRelease)
			thyme_release = cwevent->double_click = 1;
		}
	    if (xevent->type == ButtonPress)
		thyme_press = xevent->xbutton.time;
	    else
		thyme_release = xevent->xbutton.time;
	}
	x = xevent->xbutton.x;
	y = xevent->xbutton.y;
	break;
    case MotionNotify:
	x = cwevent->x = xevent->xmotion.x;
	y = cwevent->y = xevent->xmotion.y;
	break;
    }
    window = xevent->xany.window;
}

static long lower_window_callback (CWidget * w)
{E_
    if (w->position & WINDOW_ALWAYS_LOWERED)
	XLowerWindow (CDisplay, w->winid);
    return 0;
}

/*sends all windows that are marked top_bottom = 1 to the bottom */
void CLowerWindows (void)
{E_
    for_all_widgets ((for_all_widgets_cb_t) lower_window_callback, 0, 0);
}

static long raise_window_callback (CWidget * w)
{E_
    if (w->position & WINDOW_ALWAYS_RAISED)
	XRaiseWindow (CDisplay, w->winid);
    return 0;
}

/*sends all windows that are marked top_bottom = 2 to the top */
void CRaiseWindows (void)
{E_
    for_all_widgets ((for_all_widgets_cb_t) raise_window_callback, 0, 0);
}



/* {{{ here is an internal event queue handler to send events without going through XLib */

/*
   We want to be able to send our own events internally
   because XSendEvent sometimes doesn't force the an
   event to be processed and the event sits on the queue
   with its thumb up its arse.
 */

#define NUM_EVENTS_CACHED 256

static unsigned char event_send_last = 0;
static unsigned char event_read_last = 0;
static XEvent event_sent[NUM_EVENTS_CACHED];

/* returns 0, if buffer is full, else returns 1 */
int push_event (XEvent * ev)
{E_
    unsigned char j = event_send_last + 1;
    if (event_read_last == j) {		/* no more space */
#ifdef DEBUG
/* NLS ? */
	fprintf (stderr, "%s:%s:%d: *Warning* event stack full\n", PACKAGE, __FILE__, __LINE__);
#endif
	/* we are just going to ignore this */
	return 0;
    }
    if (ev->type == Expose || ev->type == InternalExpose) {	/* must handle expose counts also */
	unsigned char i = event_send_last - 1;
	XEvent *e;
	j = event_read_last - 1;
	ev->xexpose.count = 0;	/* this is the very last expose by definition */
	while (i != j) {	/* search backwards until a similar event is found */
	    if ((e = &(event_sent[i]))->xany.window == ev->xany.window) {
		if (e->type == ev->type) {
		    e->xexpose.count = 1;	/* we are not going to actually "count", but we must indicate if the queue isn't empty with a "1" */
		    break;
		}
	    }
	    i--;
	}
    }
    memcpy (&event_sent[event_send_last], ev, sizeof (XEvent));
    event_send_last++;
    return 1;
}

extern int block_push_event;	/* see initapp.c */

/* pops the oldest event, returns 0 if empty */
int pop_event (XEvent * ev)
{E_
    if (event_read_last == event_send_last)
	return 0;		/* "stack" is empty */
    block_push_event = 1;
    memcpy (ev, &event_sent[event_read_last], sizeof (XEvent));
    memset (&event_sent[event_read_last], 0, sizeof (XEvent));
    event_read_last++;
    block_push_event = 0;
    return 1;
}

/* use this instead of XSextEvent to send an event to your own application */
int CSendEvent (XEvent * e)
{E_
    int r = 0;
    block_push_event = 1;
    r = push_event (e);
    block_push_event = 0;
    return r;
}

/* sends an event (type = msg) to the given widget */
int CSendMessage (CWidget * w, int msg)
{E_
    CEvent cwevent;
    XEvent xevent;

    if (!w)
	return 0;
    memset (&cwevent, 0, sizeof (CEvent));
    memset (&xevent, 0, sizeof (XEvent));

    xevent.type = cwevent.type = msg;
    cwevent.kind = w->kind;
    xevent.xany.window = cwevent.window = w->winid;

    cwevent.ident = "";
    return run_callbacks (w, &xevent, &cwevent);
}

int CQueueSize ()
{E_
    int r = event_read_last;
    int s = event_send_last;
    s -= r;
    if (s < 0)
	return NUM_EVENTS_CACHED + s;
    return s;
}

/* returns nonzero if pending on either internal or X queue */
int CPending ()
{E_
    if (CQueueSize ())
	return 1;
    if (XEventsQueued (CDisplay, QueuedAfterFlush))
	return 1;
    return 0;
}

/* 
   does checks for expose events pending, if there is one on either queue then
   it removes it and returns it.
 */
int CExposePending (Window w, XEvent * ev)
{E_
    XEvent *e;
    int r;
    unsigned char i = event_read_last;

    while (i != event_send_last) {
	if ((e = &(event_sent[i]))->xany.window == w)
	    if (e->type == Expose) {
		memcpy (ev, e, sizeof (XEvent));
		e->type = 0;
		return 1;
	    }
	i++;
    }
    memset(ev, '\0', sizeof(*ev));
    r = XCheckWindowEvent (CDisplay, w, ExposureMask, ev);
    return r && ev->type == Expose;
}

long CUserInputEventToMask (int event_type)
{E_
    unsigned long event_mask = 0;

    if (event_type == ButtonPress)
        event_mask |= ButtonPressMask;
    if (event_type == ButtonRelease)
        event_mask |= ButtonReleaseMask;
    if (event_type == MotionNotify)
        event_mask |= ButtonMotionMask;
    if (event_type == KeyPress)
        event_mask |= KeyPressMask;
    if (event_type == KeyRelease)
        event_mask |= KeyPressMask;     /* window draws are sometimes on key release, but the queue will have tells of key presses, to indicate the user has repeat key on */

    return event_mask;
}

int CCheckSimilarEventsPending (Window w, int event_type, int do_sync)
{E_
    long mask;
    mask = CUserInputEventToMask (event_type);
    if (!mask)
        return 0;
    return CCheckWindowEvent (w, mask, do_sync);
}

/* 
   Searches the local queue for an event matching the window
   and mask. Does NOT remove the event, returns non-zero if found.
   a mask of 0 matches any type, only the masks below are supported:
   add more masks as needed. A window of 0 matches any window.
   event_return may be passed as 0 if unneeded.
 */
int CCheckWindowEvent (Window w, long event_mask, int do_sync)
{E_
    XEvent ** event_return = 0;
    unsigned char i = event_send_last - 1;
    unsigned char j = event_read_last - 1;
    static XEvent e;
    static long mask[CLASTEvent] =
    {99};

    memset (&e, 0, sizeof (e));

    if (!event_mask)
	event_mask = 0xFFFF;
    if (mask[0] == 99) {
	memset (mask, 0, CLASTEvent * sizeof (long));
	mask[KeyPress] = KeyPressMask;
	mask[KeyRelease] = KeyReleaseMask;
	mask[ButtonPress] = ButtonPressMask;
	mask[ButtonRelease] = ButtonReleaseMask;
	mask[MotionNotify] = PointerMotionMask | ButtonMotionMask;
	mask[EnterNotify] = EnterWindowMask;
	mask[LeaveNotify] = LeaveWindowMask;
	mask[Expose] = ExposureMask;
	mask[ButtonRepeat] = ButtonReleaseMask | ButtonPressMask;
    }
    while (i != j) {
	if ((event_sent[i].xany.window == w || !w)
	    && (mask[event_sent[i].type] & event_mask)) {
	    if (event_return)
		*event_return = &(event_sent[i]);
	    return 1;
	}
	i--;
    }

    if (do_sync)
        XSync (CDisplay, 0);

    if (!w) {
	if (XCheckMaskEvent (CDisplay, event_mask, &e))
	    goto send_event;
    } else {
	if (XCheckWindowEvent (CDisplay, w, event_mask, &e))
	    goto send_event;
    }
    return 0;

  send_event:
    CSendEvent (&e);
    if (event_return)
	*event_return = &e;
    return 1;
}

int CKeyPending (void)
{E_
    return CCheckWindowEvent (0, KeyPressMask, 1);
}

/* send an expose event via the internal queue */
int CSendExpose (Window win, int x, int y, int w, int h)
{E_
    XEvent e;
    memset (&e, 0, sizeof (e));
    e.xexpose.type = Expose;
    e.xexpose.serial = 0;
    e.xexpose.send_event = 1;
    e.xexpose.display = CDisplay;
    e.xexpose.window = win;
    e.xexpose.x = x;
    e.xexpose.y = y;
    e.xexpose.width = w;
    e.xexpose.height = h;
    return CSendEvent (&e);
}


/* }}} end of internal queue handler */


/* {{{ here is an expose caching-amalgamating stack system */

typedef struct {
    short x1, y1, x2, y2;
    Window w;
    long error;
    int count;
} CRegion;
    
#define MAX_NUM_REGIONS 63
static CRegion regions[MAX_NUM_REGIONS + 1];

#define area(c) abs(((c).x1-(c).x2)*((c).y1-(c).y2))

static CRegion add_regions (CRegion r1, CRegion r2)
{E_
    CRegion r;
    memset(&r, '\0', sizeof(r));
    r.x2 = max (max (r1.x1, r1.x2), max (r2.x1, r2.x2));
    r.x1 = min (min (r1.x1, r1.x2), min (r2.x1, r2.x2));
    r.y2 = max (max (r1.y1, r1.y2), max (r2.y1, r2.y2));
    r.y1 = min (min (r1.y1, r1.y2), min (r2.y1, r2.y2));
    r.w = r2.w;
    r.error = (long) area (r) - area (r1) - area (r2);
    r.error = max (r.error, 0);
    r.error += r1.error + r2.error;
    r.count = min (r1.count, r2.count);
    return r;
}

/* returns 1 when the stack is full, 0 otherwise */
static int push_region (XExposeEvent * e)
{E_
    CRegion p;

    memset(&p, '\0', sizeof(p));

    p.x1 = e->x;
    p.x2 = e->x + e->width;
    p.y1 = e->y;
    p.y2 = e->y + e->height;
    p.w = e->window;
    p.error = 0;
    p.count = e->count;

    if (last_region) {		/* this amalgamates p with a region on the stack of the same window */
	CRegion q;
	int i;
        memset(&q, '\0', sizeof(q));
	for (i = last_region - 1; i >= 0; i--) {
	    if (regions[i].w == p.w) {
		q = add_regions (regions[i], p);
		if (q.error < 100) {
		    regions[i] = q;	/* amalgamate region, else... */
		    return 0;
		}
	    }
	}
    }

    regions[last_region++] = p;		/* ...store a completely new region */
    if (last_region >= MAX_NUM_REGIONS) {
/* NLS ? */
	printf ("push_region(): last_region >= MAX_NUM_REGIONS\n");
	return 1;
    }
    return 0;
}

/*
   Pops the first region matching w, if w == 0 then pops 
   the first region, returns 1 on empty.
 */
static int pop_region (XExposeEvent * e, Window w)
{E_
    e->type = 0;
    if (last_region) {
	int i = 0;
	if (w == 0)
	    goto any_window;
	for (i = last_region - 1; i >= 0; i--) {
	    if (regions[i].w == w) {
	      any_window:;
		e->type = Expose;
		e->serial = e->send_event = 0;
		e->display = CDisplay;
		e->window = regions[i].w;
		e->x = min (regions[i].x1, regions[i].x2);
		e->y = min (regions[i].y1, regions[i].y2);
		e->width = abs (regions[i].x1 - regions[i].x2);
		e->height = abs (regions[i].y1 - regions[i].y2);
		e->count = regions[i].count;
		last_region--;
		Cmemmove (&(regions[i]), &(regions[i + 1]), (last_region - i) * sizeof (CRegion));
		return 0;
	    }
	}
    }
    return 1;
}

static void pop_all_regions (Window w)
{E_
    XEvent e;
    memset (&e, 0, sizeof (e));
    while (!pop_region (&(e.xexpose), w)) {
	e.type = InternalExpose;
	CSendEvent (&e);
    }
}


/* }}} end expose amalgamation stack system */


/* {{{ key conversion utilities */

/* xim.c */

KeySym CKeySym (XEvent * e)
{E_
    return key_sym_xlat (e, 0, 0);
}

int mod_type_key (KeySym x)
{E_
    switch ((int) x) {
#ifdef XK_Shift_L
    case XK_Shift_L:
#endif
#ifdef XK_Shift_R
    case XK_Shift_R:
#endif
#ifdef XK_Control_L
    case XK_Control_L:
#endif
#ifdef XK_Control_R
    case XK_Control_R:
#endif
#ifdef XK_Caps_Lock
    case XK_Caps_Lock:
#endif
#ifdef XK_Shift_Lock
    case XK_Shift_Lock:
#endif
#ifdef XK_Meta_L
    case XK_Meta_L:
#endif
#ifdef XK_Meta_R
    case XK_Meta_R:
#endif
#ifdef XK_Alt_L
    case XK_Alt_L:
#endif
#ifdef XK_Alt_R
    case XK_Alt_R:
#endif
#ifdef XK_Super_L
    case XK_Super_L:
#endif
#ifdef XK_Super_R
    case XK_Super_R:
#endif
#ifdef XK_Hyper_L
    case XK_Hyper_L:
#endif
#ifdef XK_Hyper_R
    case XK_Hyper_R:
#endif
	return 1;
    }
    return 0;
}


/* get a 15 bit "almost unique" key sym that includes keyboard modifier
   info in the top 3 bits */
short CKeySymMod (XEvent * ev)
{E_
    KeySym p;
    XEvent e;
    int state;
    if (!ev)
	return 0;
    e = *ev;
    state = e.xkey.state;
    e.xkey.state = 0;		/* want the raw key */
    CDisableXIM();
    p = CKeySym (&e);
    CEnableXIM();
    if (p && !mod_type_key (p)) {
	if (state & ShiftMask)
	    p ^= 0x1000;
	if (state & ControlMask)
	    p ^= 0x2000;
	if (state & MyAltMask)
	    p ^= 0x4000;
	p &= 0x7FFF;
    } else
	p = 0;
    return p;
}

/* }}} key conversion utilities */


/* {{{ focus cycling */

/* returns 1 if the key pressed is usually a key to goto next focus */
int is_focus_change_key (KeySym k, int command)
{E_
    return (k == XK_Tab || k == XK_KP_Tab || k == XK_ISO_Left_Tab || k == XK_Down ||
	k == XK_Up || k == XK_Left || k == XK_Right || k == XK_KP_Down ||
	    k == XK_KP_Up || k == XK_KP_Left || k == XK_KP_Right || command == CK_Right
	    || command == CK_Left || command == CK_Tab || command == CK_Up || command == CK_Down);
}

/* returns 1 if the key pressed is usually a key to goto previous focus */
int is_focus_prev_key (KeySym k, int command, unsigned int state)
{E_
    return (k == XK_ISO_Left_Tab || (((state) & ShiftMask) &&
    (k == XK_Tab || k == XK_KP_Tab || command == CK_Tab)) || k == XK_Left
	    || k == XK_Up || k == XK_KP_Left || k == XK_KP_Up || command == CK_Left || command == CK_Up);
}

/* use above to in combination to make this next function unnecesary */
/* int is_focus_next_key(KeySym k, int command, unsigned int state) */

/*
   This shifts focus to the previous or next sibling widget.
   (usually the tab key is used, but also responds to up, down,
   left and right.)
 */
static int CCheckTab (XEvent * xevent, CEvent * cwevent)
{E_
    if (xevent->type == KeyPress) {
	KeySym k;
	CWidget *w;
	k = CKeySym (xevent);
	if (!k)
	    return 0;
	if (!is_focus_change_key (k, cwevent->command))
	    return 0;

	w = CWidgetOfWindow (xevent->xany.window);

	if (!w)
	    CFocus (CFindFirstDescendent (xevent->xany.window));
	else if (!w->takes_focus)
	    CFocus (CChildFocus (w));
	else if (is_focus_prev_key (k, cwevent->command, xevent->xkey.state))
	    CFocus (CPreviousFocus (w));
	else
	    CFocus (CNextFocus (w));
	return (CGetFocus () != xevent->xany.window);	/* was handled since the focus did actually change */
    }
    return 0;
}

void click_on_widget (CWidget * w)
{E_
    XEvent e;
    CFocus (w);
    if (w->options & WIDGET_HOTKEY_ACTIVATES) {
	memset (&e, 0, sizeof (XEvent));
	e.xbutton.type = ButtonPress;
	e.xbutton.window = w->winid;
	e.xbutton.button = Button1;
	CSendEvent (&e);
	e.xbutton.type = ButtonRelease;
	CSendEvent (&e);
	e.xbutton.type = LeaveNotify;
	CSendEvent (&e);
    }
}


int match_hotkey (KeySym a, KeySym b)
{E_
    if (isalpha (a & 0xFF) && isalpha (b & 0xFF) && my_lower_case (a) == my_lower_case (b))
	return 1;
    if (a == b)
	return 1;
    return 0;
}

/*
   Check for hot keys of buttons, sends a ButtonPress to the button if the key found.
 */
static int CCheckButtonHotKey (XEvent * xevent, CEvent * cwevent)
{E_
    if (xevent->type == KeyPress) {
	KeySym k;
	CWidget *w, *p;
	k = CKeySym (xevent);
        if (!k)
            return 0;
	w = CWidgetOfWindow (xevent->xany.window);
	if (!w)
	    w = CFindFirstDescendent (xevent->xany.window);
	else if (!w->takes_focus)
	    w = CChildFocus (w);
	p = w = CNextFocus (w);
	do {
	    if (!w)
		return 0;
	    if (match_hotkey (w->hotkey, k)) {
		click_on_widget (w);
		return 1;
	    }
	    w = CNextFocus (w);	/* check all sibling buttons for a hotkey */
	} while ((unsigned long) w != (unsigned long) p);
    }
    return 0;
}

static long check_hotkey_callback (CWidget * w, long k, long do_focus)
{E_
    if (w->takes_focus && !w->disabled)
	if (match_hotkey (w->hotkey, k)) {
	    if (do_focus)
		click_on_widget (w);
	    return 1;
	}
    return 0;
}

/* checks all widgets for a hotkey if alt is pressed */
int CCheckGlobalHotKey (XEvent * xevent, CEvent * cwevent, int do_focus)
{E_
    KeySym k;
    k = CKeySym (xevent);
    if (!k)
	return 0;
    if (xevent->type == KeyPress && (xevent->xkey.state & MyAltMask) && !(xevent->xkey.state & ControlMask))
	return for_all_widgets ((for_all_widgets_cb_t) check_hotkey_callback, (void *) k, (void *) (long) do_focus);
    return 0;
}


/* }}} */


static char *event_names[] =
{
    "??zero??",
    "??one??",
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify"
};

static int num_event_names = sizeof (event_names) / sizeof (num_event_names);

const char *get_event_name(int type)
{E_
    if (type < 0 || type >= num_event_names)
        return "unknown";
    return event_names[type];
}


int run_callbacks (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    static char no_ident[33] = "";
    int handled = 0;

    if (!cwevent->text)
	cwevent->text = no_ident;
    if (!cwevent->ident)
	cwevent->ident = no_ident;

#ifdef DEBUG_ENTRY
/* NLS ? */
    printf ("Calling %s: Type=%s\n", w->ident, type < num_event_names ? event_names[type] : itoa (type));
#endif
    if (w->eh) {
	char ident[33];
	int (*cb) (struct cool_widget *, XEvent *, CEvent *);
	cb = w->callback;
	strcpy (ident, w->ident);
	if (w->callback_before) {
	    handled |= (*(w->callback_before)) (w, xevent, cwevent);
	    if (w != CIdent (ident))	/* callback
	    could free widget. hack this line must go
	    here because ident could = "". so happens
	    that such widgets don't have callbacks */
		return handled;
	}
	handled |= (*(w->eh)) (w, xevent, cwevent);
	if (cb) {
	    if (w != CIdent (ident))	/* same logic */
		return handled;
	    if (cwevent->ident[0])
		handled |= (*(w->callback)) (w, xevent, cwevent);
	}
    }
#ifdef DEBUG_ENTRY
/* NLS ? */
    printf ("Returned\n");
#endif
    return handled;
}

/* sets the mapped member of widget whose window is w, returning the previous state */
static int set_mapped (Window win, int i)
{E_
    int y;
    CWidget *w;
    w = CWidgetOfWindow (win);
    if (!w)
	return i;
    y = w->mapped;
    w->mapped = i;
    return y;
}

int compose_key_pressed = 0;
int compose_key_which = 0;

static unsigned int key_board_state = 0;

unsigned int CGetKeyBoardState (void)
{E_
    return key_board_state;
}

/* returns cwevent->insert == -1 and cwevent->command == 0 if no significant key pressed */
static void translate_key (XEvent * xevent, CEvent * cwevent)
{E_
    cwevent->xlat_len = 0;
    cwevent->key = key_sym_xlat (xevent, cwevent->xlat, &cwevent->xlat_len);
    if (!cwevent->key)
	cwevent->key = XK_VoidSymbol;
    cwevent->state = xevent->xkey.state;
    if (!edit_translate_key (xevent->xkey.keycode, cwevent->key, xevent->xkey.state, &cwevent->command, cwevent->xlat, &cwevent->xlat_len)) {
	cwevent->xlat_len = 0;
	cwevent->command = 0;
    }
}

/* {{{ Toolhints */

int option_toolhint_milliseconds1 = 500;

static void render_text_ordinary (Window win, int x, int y, char *q)
{E_
    char *p;
    int h = 0;
    for (;;) {
	if (!(p = strchr (q, '\n')))
	    p = q + strlen (q);
	CImageText (win, FONT_OFFSET_X + x, FONT_OFFSET_Y + h + y, q,
		    (unsigned long) p - (unsigned long) q);
	h += FONT_PIX_PER_LINE;
	if (!*p)
	    break;
	q = p + 1;
    }
}

static int eh_toolhint (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    if (xevent->type == Expose)
	if (!xevent->xexpose.count && w->label) {
	    CSetColor (color_widget (4));
	    CSetBackgroundColor (color_widget (14));
	    render_text_ordinary (w->winid, 2, 2, w->label);
	    CSetColor (color_widget (0));
	    XDrawRectangle (CDisplay, w->winid, CGC, 0, 0, w->width - 1, w->height - 1);
	}
    return 0;
}

static void hide_toolhint (void)
{E_
    CDestroyWidget ("_toolhint");
}

static void show_toolhint (Window win, int xp, int yp)
{E_
    int width, height, x, y;
    CWidget *w, *t;
    w = CWidgetOfWindow (win);
    if (!w)
	return;
    if (w->parentid != CRoot)
	if (!CWidgetOfWindow (CGetFocus ()))
	    return;
    if (!w->toolhint)
	return;
    CDestroyWidget ("_toolhint");
    CGetWindowPosition (win, CRoot, &x, &y);
    CTextSize (&width, &height, w->toolhint);
    t = CSetupWidget ("_toolhint", CRoot, x + xp + 16,
		    y + yp - 2, width + 4, height + 4, C_TOOLHINT_WIDGET,
		      ExposureMask, color_widget(14), 0);
    t->eh = eh_toolhint;
    t->label = (char *) strdup (w->toolhint);
}

void CSetToolHint (const char *ident, const char *text)
{E_
    CWidget *w;
    if (!text)
	return;
    if (!*text)
	return;
    w = CIdent (ident);
    if (!w)
	return;
    if (w->toolhint)
	free (w->toolhint);
    w->toolhint = (char *) strdup (text);
}

/* }}} Toolhints */

/* {{{ fd selection */

#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif

struct file_des_watch {
    int fd, how;
    void (*callback) (int, fd_set *, fd_set *, fd_set *, void *);
    const char *file;
    int line;
    void *data;
};

static struct file_des_watch *watch_table[FD_SETSIZE];

static int watch_table_last = 0;

/* returns non-zero if table is full */
int _CAddWatch (char *file, int line, int fd, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *),
		int how, void *data)
{E_
    int i;
    if (!callback || fd < 0 || !how) {
	fprintf (stderr, "bad args to CAddWatch??");
	return 1;
    }
    for (i = 0; i < watch_table_last; i++) {
	if (!watch_table[i])
	    continue;
	if (watch_table[i]->callback == callback && watch_table[i]->fd == fd) {
	    watch_table[i]->how |= how;
	    return 0;
	}
    }
    for (i = 0; i < watch_table_last && watch_table[i]; i++);
    if (i >= FD_SETSIZE) {
	fprintf (stderr, "watch table overflow??");
	return 1;
    }
    watch_table[i] = malloc (sizeof (struct file_des_watch));
    watch_table[i]->callback = callback;
    watch_table[i]->how = how;
    watch_table[i]->fd = fd;
    watch_table[i]->data = data;
    watch_table[i]->file = file;
    watch_table[i]->line = line;
    if (watch_table_last < i + 1)
	watch_table_last = i + 1;
    return 0;
}

void CRemoveWatch (int fd, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how)
{E_
    int i;
    for (i = 0; i < watch_table_last; i++) {
	if (!watch_table[i])
	    continue;
	if (watch_table[i]->callback == callback && watch_table[i]->fd == fd) {
	    watch_table[i]->how &= ~how;
	    if (!watch_table[i]->how)
		goto do_free;
	    return;
	}
    }
    return;
  do_free:
    free (watch_table[i]);
    watch_table[i] = 0;
    for (;;) {
	if (!watch_table_last)
	    break;
	if (watch_table[watch_table_last - 1])
	    break;
	watch_table_last--;
    }
}

void remove_all_watch (void)
{E_
    int i;
    for (i = 0; i < watch_table_last; i++) {
	if (!watch_table[i])
	    continue;
	free (watch_table[i]);
	watch_table[i] = 0;
    }
    watch_table_last = 0;
}

static int CIdle = 0;

int CIsIdle (void)
{E_
    return CIdle;
}

extern void _alarmhandler (void);
extern int got_alarm;

static int run_watches (void)
{E_
    int r, n = 0, i, found_watch;
    fd_set reading, writing, error;
    FD_ZERO (&reading);
    FD_ZERO (&writing);
    FD_ZERO (&error);
    FD_SET (ConnectionNumber (CDisplay), &reading);
    n = max (n, ConnectionNumber (CDisplay));
    for (i = 0; i < watch_table_last; i++) {
	if (!watch_table[i])
	    continue;
	if (watch_table[i]->how & WATCH_READING) {
	    FD_SET (watch_table[i]->fd, &reading);
	    n = max (n, watch_table[i]->fd);
	}
	if (watch_table[i]->how & WATCH_WRITING) {
	    FD_SET (watch_table[i]->fd, &writing);
	    n = max (n, watch_table[i]->fd);
	}
	if (watch_table[i]->how & WATCH_ERROR) {
	    FD_SET (watch_table[i]->fd, &error);
	    n = max (n, watch_table[i]->fd);
	}
    }
#if 0	/* Norbert Nemec <nobbi@cheerful.com> */
    if (!n)
	return 0;
#endif	/* Norbert Nemec <nobbi@cheerful.com> */
    r = select (n + 1, &reading, &writing, &error, 0);
    if (got_alarm)
	_alarmhandler ();
    childhandler_ ();
    if (r <= 0)
	return 0;
    if (FD_ISSET (ConnectionNumber (CDisplay), &reading)) {
	CIdle = 0;
	r = 1;
    } else {
	CIdle = 1;
	r = 0;
    }
/* callbacks could add or remove watches, so we restart the loop after each callback */
    do {
	found_watch = 0;
	for (i = 0; i < watch_table_last; i++) {
	    int fd;
	    if (!watch_table[i])
		continue;
	    fd = watch_table[i]->fd;
	    if (FD_ISSET (fd, &reading) && watch_table[i]->how & WATCH_READING) {
		(*watch_table[i]->callback) (fd, &reading, &writing, &error, watch_table[i]->data);
		FD_CLR (fd, &reading);
		found_watch = 1;
		break;
	    }
	    if (FD_ISSET (fd, &writing) && watch_table[i]->how & WATCH_WRITING) {
		(*watch_table[i]->callback) (fd, &reading, &writing, &error, watch_table[i]->data);
		FD_CLR (fd, &writing);
		found_watch = 1;
		break;
	    }
	    if (FD_ISSET (fd, &error) && watch_table[i]->how & WATCH_ERROR) {
		(*watch_table[i]->callback) (fd, &reading, &writing, &error, watch_table[i]->data);
		FD_CLR (fd, &error);
		found_watch = 1;
		break;
	    }
	}
    } while (found_watch);
    return r;
}

/* }}} fd selection */

extern Atom ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW, ATOM_WM_TAKE_FOCUS;

/*
   This is the core of the library. CNextEvent must be called continually.
   The calling application must only use CNextEvent as a block.
   CNextEvent does the following in sequence:

   1 check if event is AlarmEvent, yes: toggle cursor and return
   2 check MappingNotify and return
   3 cache expose event for later handling. No "raw" exposes are ever processed.
   they are first merged into courser exposes and resent via the internal
   queue; return
   4 check if an internal expose resulting from 3 above. If so rename it to Expose
   and continue
   5 Check for various other events and process/convert them
   6 look for a widget whose window matches .xany.window. If the widget is not
   a picture widget then call the widget event handling routine: eh_*
   Then call the widgets user event handler: ->callback
   7 do the same for picture widgets. These must come last so that the get drawn
   on top of other things if there is for exampla a image and a picture in
   the same window.
   8 if the event was a key event, and none of the handlers returned 1
   check the tab key for focus cycling.
   
   This returns cwevent->handled non-zero if the event was a key event and
   was handled, 0 otherwise.
 */

int (*global_callback) (XEvent *x) = 0;
extern char current_pulled_button[33];
extern int font_depth;

void CNextEvent_check_font (XEvent * xevent, CEvent * cwevent);

void CNextEvent (XEvent * xevent, CEvent * cwevent)
{E_
    int d;
    d = font_depth;
    CNextEvent_check_font (xevent, cwevent);
    if (d != font_depth) {
        CWidget *w = 0;
        fprintf(stderr, "font depth mismatch %d vs %d: ", d, font_depth);
        if (xevent->xany.window)
            w = CWidgetOfWindow(xevent->xany.window);
        fprintf(stderr, "ident = %s, event %d\n", w ? w->ident : "<unknown>", (int) xevent->xany.type);
    }
}

/* xevent or cwevent or both my be passed as NULL */
void CNextEvent_check_font (XEvent * xevent, CEvent * cwevent)
{E_
    static char idle = 1;
    static char no_ident[33];
    int i = 0;
    int handled = 0;
    CWidget *w = 0;
    XEvent private_xevent;
    CEvent private_cwevent;
    Window win;
#ifdef HAVE_DND
    static Window drop_window = 0;
#endif
    int type;
    static Window last_events_window1 = -2;
    static Window last_events_window2 = -2;
    static int last_widget1 = 0;
    static int last_widget2 = 0;
    static XEvent button_repeat;
    static int button_repeat_count = 0;
    static Window toolhint_window = 0;
    static int toolhint_count = 0, x_toolhint, y_toolhint;
    int next_event = 0;

    memset (&button_repeat, 0, sizeof (button_repeat));

    if (!xevent) {
        memset(&private_xevent, '\0', sizeof(private_xevent));
	xevent = &private_xevent;
    }
    if (!cwevent) {
        memset(&private_cwevent, '\0', sizeof(private_cwevent));
	cwevent = &private_cwevent;
    }

    if (!CPending ())		/* flush output; check if events */
	pop_all_regions (0);	/* just make sure not outstanding exposes */
    while (!pop_event (xevent)) {	/* first check our own events, if none of our own coming, _then_ we look at the server */
	if (QLength (CDisplay)) {
	    memset (xevent, 0, sizeof (XEvent));
	    XNextEvent (CDisplay, xevent);
	    next_event = 1;
	    break;
	} else if (run_watches ()) {
	    memset (xevent, 0, sizeof (XEvent));
	    XNextEvent (CDisplay, xevent);
	    next_event = 1;
	    break;
	}
    }

    memset (cwevent, 0, sizeof (CEvent));
    memset (no_ident, 0, 33);
    cwevent->text = no_ident;
    cwevent->ident = no_ident;

/* used for rxvt stuff */
    if (global_callback)
	if ((*global_callback) (xevent))
	    return;

    if (next_event && xevent->xany.type != KeyRelease) {
#warning backport this fix: change to None
	if (XFilterEvent(xevent, None)) {
	    if (xevent->xany.type == KeyPress)
	        cim_check_spot_change (xevent->xany.window);
	    xevent->xany.type = 0;
	    xevent->xany.window = 0;
	    cwevent->type = 0;
	    cwevent->kind = 0;
	    return;
	}
    }

    win = xevent->xany.window;
    type = xevent->type;

#if 0
if (type && type < 37 && type != 14)
printf("event type=%d\n", type);
#endif

    switch (type) {
    case TickEvent:
	if (idle == 1)		/* this will XSync at the end of a burst of events */
	    XSync (CDisplay, 0);	/* this, dnd.c and CKeyPending above are the only places in the library where XSync is called */
	if (button_repeat_count++ > 10)
	    if (button_repeat.type == ButtonRepeat)
		if (!(button_repeat_count % (ALRM_PER_SECOND / 25)))
		    CSendEvent (&button_repeat);
	if (toolhint_window && option_toolhint_milliseconds1 && !current_pulled_button[0]) {
	    toolhint_count++;
	    if (toolhint_count > (ALRM_PER_SECOND * option_toolhint_milliseconds1) / 1000) {
		show_toolhint (toolhint_window, x_toolhint, y_toolhint);
		toolhint_window = 0;
	    }
	}
	idle++;
	return;
    case AlarmEvent:
	{
	    int a;
	    xevent->type = cwevent->type = AlarmEvent;
	    toggle_cursor ();
	    for (a = 0; a < 33; a++) {
		if (global_alarm_callback[a]) {
		    cwevent->type = type;
		    cwevent->kind = C_ALARM_WIDGET;
		    (*(global_alarm_callback[a])) (0, xevent, cwevent);
		}
	    }
	}
	return;
    case MappingNotify:
	XRefreshKeyboardMapping (&(xevent->xmapping));
	break;
    case Expose:{
	    XEvent eev;
	    memset(&eev, '\0', sizeof(eev));
/* here we amalgamate exposes of the same window together and re-send them as InternalExpose events */
	    if (push_region (&(xevent->xexpose))) {
		pop_all_regions (win);
	    } else {
		for (;;) {
		    if (CExposePending (win, &eev)) {
			if (!push_region (&(eev.xexpose)))
			    continue;
		    }
		    pop_all_regions (win);
		    break;
		}
	    }
	}
	return;
    case InternalExpose:
	type = xevent->type = Expose;
	if (!xevent->xexpose.count)
	    render_focus_border (win);
	break;
    case EnterNotify:
/* The dnd drag will trap all events except enter and leave. These can
   be used to trace which window the pointer has gotten into during
   a drag. */
	toolhint_count = 0;
	toolhint_window = xevent->xbutton.window;
	hide_toolhint ();
	x_toolhint = xevent->xbutton.x;
	y_toolhint = xevent->xbutton.y;
#ifdef HAVE_DND
	drop_window = xevent->xbutton.window;
#endif
	break;
    case LeaveNotify:
	toolhint_window = 0;
	hide_toolhint ();
	break;
    case MapNotify:
	if (set_mapped (xevent->xmap.window, WINDOW_MAPPED) & WINDOW_FOCUS_WHEN_MAPPED)
	    focus_window (xevent->xmap.window);
	break;
    case FocusOut:
    case FocusIn:
	hide_toolhint ();
	toolhint_window = 0;
	process_external_focus (win, type);
	return;
	break;
    case MotionNotify:
#if 0
	if (xevent->xmotion.button == Button2 && option_middle_button_pastes) {
	    break;
	}
#endif
	if (xevent->xmotion.window == toolhint_window) {
	    hide_toolhint ();
	    x_toolhint = xevent->xmotion.x;
	    y_toolhint = xevent->xmotion.y;
	}
	break;
    case ButtonPress:
	if (xevent->xbutton.button == Button2 && option_middle_button_pastes) {
	    xevent->type = KeyPress;
	    cwevent->command = CK_XPaste;
	    cwevent->xlat_len = 0;
	}
	key_board_state = xevent->xbutton.state;
	hide_toolhint ();
	toolhint_window = 0;
	memcpy (&button_repeat, xevent, sizeof (XEvent));
	button_repeat.type = ButtonRepeat;
	button_repeat_count = 0;
	break;
    case ButtonRelease:
	if (xevent->xbutton.button == Button2 && option_middle_button_pastes) {
	    break;
	}
	key_board_state = xevent->xbutton.state;
	toolhint_window = 0;
	button_repeat.type = 0;
	break;
    case KeyPress:
	key_board_state = xevent->xkey.state;
	hide_toolhint ();
	toolhint_window = 0;
	translate_key (xevent, cwevent);
    case KeyRelease:
	key_board_state = xevent->xkey.state;
	win = xevent->xkey.window = CGetFocus ();
	break;
    case ConfigureNotify:{
	    CWidget *m;
	    m = CWidgetOfWindow (win);
	    if (!m)
		m = CFindFirstDescendent (win);
	    if (!m)
		return;
	    if (m->parentid != CRoot)
		return;
	    CSetSize (m, xevent->xconfigure.width, xevent->xconfigure.height);
#if 0
printf("CRoot=%lu parentid=%lu\n", (unsigned long) CRoot, (unsigned long) m->parentid);
#endif
	    cim_send_spot (m->winid);
	}
	return;
    case SelectionNotify:
	if (xdnd_handle_drop_events (CDndClass, xevent))
	    return;
	break;
    case SelectionClear:
	selection_clear ();
	return;
    case UnmapNotify:
	set_mapped (xevent->xmap.window, 0);
	break;
    case ClientMessage:
#ifdef HAVE_DND
/* If we recieve a drop from dnd, we need to find the window in which the
   drop occurred. This will be the last window with an EnterNotify (above).
   Now we find the pointer coords relative to that window, and change the
   event to go to that window */
	if ((xevent->xclient.message_type == DndProtocol && xevent->xclient.data.l[4] == 1)
	    ||
	    (xevent->xclient.message_type == OldDndProtocol && xevent->xclient.data.l[4] == 0)) {
	    int x, y, rx, ry;
	    Window root, child;
	    unsigned int mask;
	    win = xevent->xclient.window = drop_window;
	    XQueryPointer (CDisplay, drop_window, &root, &child, &rx, &ry, &x, &y, &mask);
	    xevent->xclient.data.l[3] = (long) x + (long) y *65536L;
	}
#else
	if (xdnd_handle_drop_events (CDndClass, xevent))
	    return;
#endif
	if (xevent->xclient.message_type == ATOM_WM_PROTOCOLS) {
	    if (xevent->xclient.data.l[0] == ATOM_WM_DELETE_WINDOW) {
		if (xevent->xclient.window == CFirstWindow) {
		    XEvent exit_event;
		    memset (&exit_event, 0, sizeof (exit_event));
		    exit_event.type = QuitApplication;
		    CSendEvent (&exit_event);
		} else {
		    CDestroyWidget ((CWidgetOfWindow (xevent->xclient.window))->ident);
		}
		return;
	    }
	    if (xevent->xclient.data.l[0] == ATOM_WM_TAKE_FOCUS) {
		hide_toolhint ();
		toolhint_window = 0;
		process_external_focus (win, type);
		return;
	    }
	}
	break;
    }
    idle = 0;

    if (last_events_window1 == win && CIndex (last_widget1))	/* this will speed up the search a bit */
	i = last_widget1 - 1;	/* by remembering the last two windows */
    else if (last_events_window2 == win && CIndex (last_widget2))
	i = last_widget2 - 1;

/* Now find if the event belongs to any of the widgets */
    while (last_widget > i++) {
	if (!(w = CIndex (i)))
	    continue;
	if (w->winid != win)
	    continue;
	if (w->disabled)
	    if (type != Expose && type != FocusOut && type != SelectionRequest && type != LeaveNotify && type != ClientMessage)
		break;
	if (w->kind == C_PICTURE_WIDGET)
	    continue;

	last_widget2 = last_widget1;
	last_widget1 = i;
	last_events_window2 = last_events_window1;
	last_events_window1 = win;

	cwevent->type = type;
	cwevent->kind = w->kind;
	cwevent->window = win;

	handled |= run_callbacks (w, xevent, cwevent);
	w = CIndex (i);
	break;
    }

#ifdef HAVE_PICTURE

    i = 0;
/* picture exposes must come last so that they can be drawn on top of
   other widgets */
    if (type == Expose && w) {
	while (last_widget > i++) {
	    if (!(w = CIndex (i)))
		continue;
	    if (w->kind != C_PICTURE_WIDGET)
		continue;
	    if (w->parentid != xevent->xany.window)
		continue;
	    if (w->disabled && type != Expose)
		continue;
	    cwevent->type = type;
	    cwevent->kind = w->kind;
	    cwevent->window = xevent->xany.window;

	    handled |= run_callbacks (w, xevent, cwevent);
	    /*break; *//*no break here 'cos there may be two picture widgets in the same window */
	}
    }
#endif				/* ! HAVE_PICTURE */

    if (xevent->type == KeyPress && w) {
	cwevent->handled = handled;
	if (!handled)
	    handled = CHandleGlobalKeys (w, xevent, cwevent);
    }

    (void) handled;

#ifdef DEBUG_DOUBLE
    widget_check_magic ();
#endif

    if (!cwevent->text)
	cwevent->text = no_ident;
    if (!cwevent->ident)
	cwevent->ident = no_ident;

    return;
}


int CHandleGlobalKeys (CWidget *w, XEvent * xevent, CEvent * cwevent)
{E_
    int handled = 0;

    if (!handled)
        handled = CCheckTab (xevent, cwevent);
    if (!handled)
        handled = CCheckButtonHotKey (xevent, cwevent);
    if (!handled)
        handled = CCheckGlobalHotKey (xevent, cwevent, 1);

    return handled;
}


int inbounds (int x, int y, int x1, int y1, int x2, int y2);


/*-----------------------------------------------------------------------*/
int eh_button (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    static Window last_win = 0;
    switch (xevent->type) {
    case MotionNotify:
	break;
    case ButtonPress:
	last_win = xevent->xbutton.window;
	if (xevent->xbutton.button == Button1 || xevent->xbutton.button == Button2
	    || xevent->xbutton.button == Button3) {
	    w->options &= (0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT);
	    w->options |= BUTTON_PRESSED;
	    CFocus (w);
	    (*w->render) (w);
	}
	break;
    case KeyPress:
	if ((cwevent->command != CK_Enter || w->kind == C_SWITCH_WIDGET)
	    && cwevent->key != XK_space)
	    break;
	w->options &= (0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT);
	w->options |= BUTTON_PRESSED;
	if (w->kind == C_SWITCH_WIDGET)
	    toggle_radio_button (w);
	cwevent->ident = w->ident;	/* return the event */
	(*w->render) (w);
	return 1;
    case KeyRelease:
	w->options &= (0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT);
	(*w->render) (w);
	break;
    case ButtonRelease:
	last_win = 0;
	if (xevent->xbutton.button == Button1 || xevent->xbutton.button == Button2
	    || xevent->xbutton.button == Button3) {
	    w->options &= (0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT);
	    w->options |= BUTTON_HIGHLIGHT;
	    if (inbounds (xevent->xbutton.x, xevent->xbutton.y, 0, 0, w->width, w->height)) {
		if (w->kind == C_SWITCH_WIDGET)
		    toggle_radio_button (w);
		cwevent->ident = w->ident;	/* return the event */
		(*w->render) (w);
		return 1;
	    }
	    (*w->render) (w);
	}
	return 0;
    case EnterNotify:
	w->options &= ~(BUTTON_PRESSED | BUTTON_HIGHLIGHT);
	w->options |= BUTTON_HIGHLIGHT | (last_win == xevent->xbutton.window ? BUTTON_PRESSED : 0);
	(*w->render) (w);
	break;
    case Expose:
	if (xevent->xexpose.count)
	    break;
	(*w->render) (w);
	break;
    case LeaveNotify:
	w->options &= (0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT);
	(*w->render) (w);
	break;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
int eh_bitmap (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
    case Expose:
	if (!xevent->xexpose.count)
	    render_button (w);
	break;
    }
    return 0;
}

extern struct look *look;

/*-----------------------------------------------------------------------*/
int eh_window (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    return (*look->window_handler) (w, xevent, cwevent);
}

/*-----------------------------------------------------------------------*/
int eh_bar (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
#if 0
    case ResizeNotify:	
	CSetWidgetSize (w->ident, xevent->xconfigure.width - WIDGET_SPACING * 2, 3);
	break;
#endif
    case Expose:
	if (!xevent->xexpose.count)
	    render_bar (w);
	break;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
int eh_progress (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
    case Expose:
	if (!xevent->xexpose.count)
	    render_progress (w);
	break;
    }
    return 0;
}


/*-----------------------------------------------------------------------*/
int eh_status (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
    case Expose:
	if (!xevent->xexpose.count)
	    render_status (w, 1);
	break;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
int eh_text (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
    case Expose:
	if (!xevent->xexpose.count)
	    render_text (w);
	break;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
int eh_sunken (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
    case Expose:
	if (!xevent->xexpose.count)
	    render_sunken (w);
	break;
    }
    return 0;
}
/*-----------------------------------------------------------------------*/
int eh_bwimage (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
/*      case C_8BITIMAGE_WIDGET:
   case C_BWIMAGE_WIDGET: */
#ifdef HAVE_BWIMAGE
    switch (xevent->type) {
    case Expose:
	render_bw_image (w, xevent->xexpose.x, xevent->xexpose.y, xevent->xexpose.width, xevent->xexpose.height);
	break;
    case ButtonRelease:
    case ButtonPress:
    case MotionNotify:
	resolve_button (xevent, cwevent);
	cwevent->x -= 2;	/*subtract border */
	cwevent->y -= 2;
	cwevent->ident = w->ident;
	break;
    }
#endif
    return 0;
}

/*-----------------------------------------------------------------------*/

extern int eh_textbox (CWidget * w, XEvent * xevent, CEvent * cwevent);
extern int eh_textinput (CWidget * w, XEvent * xevent, CEvent * cwevent);
extern int eh_scrollbar (CWidget * w, XEvent * xevent, CEvent * cwevent);
extern int eh_unicode (CWidget * w, XEvent * xevent, CEvent * cwevent);

int (*default_event_handler (int i)) (CWidget *, XEvent *, CEvent *) {
    switch (i) {
    case C_BITMAPBUTTON_WIDGET:
    case C_SWITCH_WIDGET:
    case C_BUTTON_WIDGET:
	return eh_button;
    case C_WINDOW_WIDGET:
	return eh_window;
    case C_BAR_WIDGET:
	return eh_bar;
    case C_SUNKEN_WIDGET:
	return eh_sunken;
    case C_HORSCROLL_WIDGET:
    case C_VERTSCROLL_WIDGET:
    case C_HORISCROLL_WIDGET:
	return eh_scrollbar;
    case C_TEXTINPUT_WIDGET:
	return eh_textinput;
    case C_TEXTBOX_WIDGET:
	return eh_textbox;
    case C_TEXT_WIDGET:
	return eh_text;
    case C_STATUS_WIDGET:
	return eh_status;
    case C_8BITIMAGE_WIDGET:
    case C_BWIMAGE_WIDGET:
	return eh_bwimage;
    case C_PROGRESS_WIDGET:
	return eh_progress;
    case C_BITMAP_WIDGET:
	return eh_bitmap;
#ifdef HAVE_PICTURE
    case C_PICTURE_WIDGET:
	return eh_picture;
#endif
    case C_EDITOR_WIDGET:
	return eh_editor;
    case C_UNICODE_WIDGET:
	return eh_unicode;
    }
    return (int (*) (CWidget *, XEvent *, CEvent *)) 0;
}

