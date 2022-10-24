/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* coolwidget.c - routines for simple widgets. Widget setup and destruction
   Copyright (C) 1996-2022 Paul Sheer
 */


#define COOL_WIDGET_C

#include "inspect.h"
#include <X11/Xatom.h>
#include "coolwidget.h"
#include "coollocal.h"
#include "stringtools.h"


extern struct look *look;


/* call this for fatal errors */
void CError (const char *fmt,...)
{E_
    va_list s;
    char *str;
    va_start (s, fmt);
    str = vsprintf_alloc (catstrs (" ", fmt, " ", NULL), s);
    CFatalErrorDialog (20, 20, str);
    va_end (s);
    free (str);
}



/* an malloc with an error check */

#ifndef CMalloc
void *CMalloc (size_t size)
{E_
    void *p;
    if ((p = malloc (size + 8)) == NULL)
/* Not essential to translate */
	CError (_("Unable to allocate memory.\n"));
    return p;
}
#endif

#ifdef DEBUG
void *CDebugMalloc (size_t x, int line, const char *file)
{E_
    void *p;
    if ((p = malloc (x)) == NULL)
/* Not essential to translate */
	CError (_("Unable to allocate memory: line %d, file %s.\n"), line, file);
    return p;
}
#endif


int allocate_color (char *color_)
{E_
    char color[MAX_X11_COLOR_NAME_LEN], *p;
    if (!color_)
	return NO_COLOR;
    if (!color_[0])
	return NO_COLOR;
    Cstrlcpy (color, color_, sizeof (color));
    if ((p = strchr (color, '/')))
        *p = '\0';
    if (*color >= '0' && *color <= '9') {
	return atoi (color);
    } else {
	int i;
	XColor c;

        /* not in rgb.txt */
        if (!strcasecmp (color, "base"))
            strcpy (color, "gray16");
        if (!strcasecmp (color, "brightblue"))
            strcpy (color, "SteelBlue2");
        if (!strcasecmp (color, "brightcyan"))
            strcpy (color, "LightCyan3");
        if (!strcasecmp (color, "brightgreen"))
            strcpy (color, "PaleGreen1");
        if (!strcasecmp (color, "brightmagenta"))
            strcpy (color, "plum3");
        if (!strcasecmp (color, "brightred"))
            strcpy (color, "IndianRed1");

        /* without this pre-check a lot of network time gets consumed looking up the same colors repeatedly */
	for (i = 0; i < color_last_pixel; i++)
            if (!strcasecmp (color, color_palette_name (i)))
		return i;
        if (color_last_pixel >= MAX_STORED_COLORS - N_WIDGET_COLORS)
	    return NO_COLOR;
	if (!XParseColor (CDisplay, CColormap, color, &c))
	    return NO_COLOR;
	if (!XAllocColor (CDisplay, CColormap, &c))
	    return NO_COLOR;
	for (i = 0; i < color_last_pixel; i++) {
	    if (color_palette (i) == c.pixel) {
                strcpy (color_palette_name (i), color);
		return i;
            }
        }
	color_palette (color_last_pixel) = c.pixel;
        strcpy (color_palette_name (color_last_pixel), color);
	return color_last_pixel++;
    }
}


struct cursor_state {
    int x, y, h, w;
    Window window;
    GC gc;
    struct aa_font aa_font;
    int state;
    int type;
    C_wchar_t chr;
    unsigned long bg, fg;
    int style;
    int font_x, font_y;
};

static struct cursor_state CursorState;

void render_cursor (struct cursor_state c);

void init_cursor_state(void)
{E_
    memset(&CursorState, '\0', sizeof(CursorState));
}

void set_cursor_position (Window win, int x, int y, int w, int h, int type, C_wchar_t chr, unsigned long bg, unsigned long fg, int style)
{E_
    if (win == CGetFocus ()) {
	CursorState.x = x;
	CursorState.y = y;
	CursorState.h = h;
	CursorState.w = w;
	CursorState.window = win;
	CursorState.gc = CGC;
	CursorState.aa_font = current_font->f;
	CursorState.type = type;
	CursorState.chr = chr;
	CursorState.style = style;
	CursorState.bg = bg;
	CursorState.fg = fg;
	CursorState.font_x = FONT_OFFSET_X;
	CursorState.font_y = FONT_OFFSET_Y;
	render_cursor (CursorState);
    } else {
	if (!(win | h | w))
	    CursorState.window = 0;
    }
}

int option_never_raise_wm_windows = 0;
int option_xor_cursor = 0;
int option_flashing_cursor = 1;
unsigned long option_cursor_color;

void render_cursor (struct cursor_state c)
{E_
    if (!CursorState.window)
	return;
    if (c.type == CURSOR_TYPE_EDITOR) {
	if (CursorState.window != CGetFocus ())
	    return;
	CPushFont ("editor", 0);
	if (!option_xor_cursor) {
	    if (c.state || !option_flashing_cursor)
		CSetColor (option_cursor_color);
	    else
		CSetColor (c.bg);
	    if (c.style & MOD_REVERSE) {
		CLine (c.window, c.x + c.w - 1, c.y + FONT_OVERHEAD, c.x + c.w - 1, c.y + c.h - 1);
		CLine (c.window, c.x + c.w - 2, c.y + FONT_OVERHEAD, c.x + c.w - 2, c.y + c.h - 1);
	    } else {
		CLine (c.window, c.x, c.y + FONT_OVERHEAD, c.x, c.y + c.h - 1);
		CLine (c.window, c.x + 1, c.y + FONT_OVERHEAD, c.x + 1, c.y + c.h - 1);
	    }
	    CLine (c.window, c.x, c.y + FONT_OVERHEAD, c.x + c.w - 1, c.y + FONT_OVERHEAD);
	    CLine (c.window, c.x, c.y + FONT_OVERHEAD + 1, c.x + c.w - 1, c.y + FONT_OVERHEAD + 1);
	}
	if (!c.state && option_flashing_cursor) {
	    XSetBackground (CDisplay, c.gc, c.bg);
	    XSetForeground (CDisplay, c.gc, c.fg);
	    CImageTextWC (c.window, c.x + c.font_x, c.y + c.font_y, 0, &(c.chr), 1);
	} else {
	    if (option_xor_cursor) {
		XSetBackground (CDisplay, c.gc, c.fg);
		XSetForeground (CDisplay, c.gc, c.bg);
		CImageTextWC (c.window, c.x + c.font_x, c.y + c.font_y, 0, &(c.chr), 1);
	    }
	}
	CPopFont ();
    } else {
	if (CursorState.window != CGetFocus ()) {
	    CSetColor (COLOR_FLAT);
	    CLine (c.window, c.x, c.y, c.x, c.y + c.h - 6);
	} else {
	    render_bevel (c.window, c.x - 1, c.y - 1, c.x, c.y + c.h - 5, 1, CursorState.state ? 0 : 1);
	}
    }
}


void CSetCursorColor (unsigned long c)
{E_
    option_cursor_color = c;
}

/* this is called from CNextEvent if an alarm event comes */
void toggle_cursor (void)
{E_
    CursorState.state = 1 - CursorState.state;
    render_cursor (CursorState);
}

void set_cursor_visible (void)
{E_
    CursorState.state = 1;
    render_cursor (CursorState);
}

void set_cursor_invisible (void)
{E_
    CursorState.state = 0;
    render_cursor (CursorState);
}


union search_hack {
    char shc[4];
    u_32bit_t shi;
    word shw;
};

/*
   These three routines are not much slower than doing the same
   thing with integers as identifiers instead of strings.
   It returns the index in the global array of widgets of
   the widget named ident. Returns 0 if not found.
 */
static int find_ident (const char *ident)
{E_
    int i = last_widget + 1;
    union search_hack d;

    if (!ident)
	return 0;
    if (!ident[0])
	return 0;
    strncpy (d.shc, ident, 4);

    if (ident[1] && ident[2]) {
/* can compare first four bytes at once */
	while (--i)
	    if (CIndex (i))
		if (((union search_hack *) CIndex (i)->ident)->shi == d.shi)
		    if (!strcmp (CIndex (i)->ident, ident))
			return i;
	return 0;
    } else {
	while (--i)
	    if (CIndex (i))
		if (((union search_hack *) CIndex (i)->ident)->shw == d.shw)
		    if (!strcmp (CIndex (i)->ident, ident))
			return i;
    }
    return 0;
}

CWidget *CIdent (const char *ident)
{E_
    return CIndex (find_ident (ident));
}

CWidget *CWidgetOfWindow (Window win)
{E_
    return CIndex (widget_of_window (win));
}

int CSystem (const char *string)
{E_
    int r;
    CDisableAlarm ();
    r = system (string);
    CEnableAlarm ();
    return r;
}

extern int (*global_alarm_callback[33]) (CWidget *, XEvent *, CEvent *);

void CAddCallback (const char *ident, int (*callback) (CWidget *, XEvent *, CEvent *))
{E_
    CWidget *w = CIdent (ident);
    if (w)
	w->callback = callback;
    else {
	if (!strcmp (ident, "AlarmCallback"))
	    global_alarm_callback[0] = callback;
	if (!strncmp (ident, "AlarmCallback", 13))
	    global_alarm_callback[atoi (ident + 13) + 1] = callback;
    }
}

void CAddBeforeCallback (const char *ident, int (*callback) (CWidget *, XEvent *, CEvent *))
{E_
    CWidget *w = CIdent (ident);
    if (w)
	w->callback_before = callback;
}

#ifdef DEBUG_DOUBLE
/* checks the magic numbers */
int widget_check_magic ()
{E_
    int i = 0;

    while (last_widget > i++)
	if (CIndex (i) != NULL)
	    if (CIndex (i)->magic_begin != WIDGET_MAGIC_BEGIN || CIndex (i)->magic_end != WIDGET_MAGIC_END)
/* NLS ? */
		CError ("Cool widget internal error - magic number overwritten overwritten.\n");
    return 0;
}
#endif

/* sends a full expose event to the widget */
void CExpose (const char *ident)
{E_
    CWidget *w = CIdent (ident);
    if (w)
	CSendExpose (w->winid, 0, 0, w->width, w->height);
}

/* Returns the widgets window or 0 if not found */
Window CWindowOfWidget (const char *ident)
{E_
    CWidget *w = CIdent (ident);
    if (w)
	return w->winid;
    else
	return 0;
}

/* send an expose event to the internel queue */
void CExposeWindowArea (Window win, int count, int x, int y, int w, int h)
{E_
    if (x < 0) {
	w = x + w;
	x = 0;
    }
    if (y < 0) {
	h = y + h;
	y = 0;
    }
    if (w <= 0 || h <= 0)
	return;

    CSendExpose (win, x, y, w, h);
}


/* Returns the first NULL list entry. Exits if list full. */
CWidget **find_empty_widget_entry ()
{E_
    int i = 0;

/* widget can be added to an empty point in the list (created from an
   undraw command, or to the end of the list. */
    while (last_widget > i++) {
	if (CIndex (i) == NULL)
	    break;
    }

    if (i == MAX_NUMBER_OF_WIDGETS - 2)
/* NLS ? */
	CError ("No more space in widget list\nIncrease MAX_NUMBER_OF_WIDGETS in coolwidget.h\n");

    if (i == last_widget)
	last_widget++;		/* increase list length if an entry was added to the end */

    return &(CIndex (i));
}


/* Fills in the widget structure. */
CWidget *allocate_widget (Window newwin, const char *ident, Window parent, int x, int y,
			  int width, int height, int kindofwidget)
{E_
    CWidget *w = CMalloc (sizeof (CWidget));
    memset (w, 0, sizeof (CWidget));	/*: important, 'cos free's check if NULL before freeing many parems */

    w->magic_begin = WIDGET_MAGIC_BEGIN;
    w->winid = newwin;
    w->parentid = parent;
    w->width = width;
    w->height = height;
    w->x = x;
    w->y = y;
    strncpy (w->ident, ident, 32);

    w->kind = kindofwidget;
    w->magic_end = WIDGET_MAGIC_END;
    return w;
}

static int override_redirect = 0;

void CSetOverrideRedirect (void)
{E_
    override_redirect = 1;
}

void CClearOverrideRedirect (void)
{E_
    override_redirect = 0;
}


/*
   Sets up the widget's window and calls allocate_widget()
   to allocate space and set up the data structures.
   What is set up here is common to all widgets, so
   it will always be the first routine called by a CDraw...()
   function.
 */
CWidget *CSetupWidget (const char *identifier, Window parent, int x, int y,
	    int width, int height, int kindofwidget, unsigned long input,
		       unsigned long bgcolor, int takes_focus)
{E_
    XSetWindowAttributes xswa;
    Window newwin;
    CWidget **w;

    memset (&xswa, '\0', sizeof (xswa));

/* NLS ? */
    if (CIdent (identifier) && kindofwidget == C_BUTTON_WIDGET)
/* Not essential to translate */
	CError (_ ("Trying to create a button with the same identifier as an existing widget.\n"));

    xswa.colormap = CColormap;
    xswa.bit_gravity = NorthWestGravity;
    xswa.background_pixel = bgcolor;
    switch (kindofwidget) {
    case C_MENU_WIDGET:
    case C_TOOLHINT_WIDGET:
    case C_ICON_WIDGET:
	xswa.override_redirect = 1;
	break;
    default:
	xswa.override_redirect = override_redirect;
	break;
    }

    newwin = XCreateWindow (CDisplay, parent, x, y, width, height, 0,
			    CDepth, InputOutput, CVisual,
	    CWOverrideRedirect | CWColormap | CWBackPixel | CWBitGravity,
			    &xswa);

    w = find_empty_widget_entry ();	/* find first unused list entry in list of widgets */
    *w = allocate_widget (newwin, identifier, parent, x, y,
			  width, height, kindofwidget);

    (*w)->mainid = CFindParentMainWindow (parent);
    (*w)->eh = default_event_handler (kindofwidget);
    (*w)->takes_focus = takes_focus;

    XSelectInput (CDisplay, newwin, input);
    switch ((*w)->kind) {
    case C_WINDOW_WIDGET:	/* window widgets must only be mapped 
				   once all there children are drawn */
#ifdef USE_XIM
	if (CIM) {
	    create_input_context (*w, get_input_style ());
	    set_status_position (*w);
	}
#endif
	break;
    default:
	XMapWindow (CDisplay, newwin);	/* shows the window */
	XFlush (CDisplay);
	break;
    }
    return (*w);
}

extern Atom ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW, ATOM_WM_TAKE_FOCUS, ATOM_WM_NAME, ATOM_WM_NORMAL_HINTS;

void CSetWindowSizeHints (CWidget * wdt, int min_w, int min_h, int max_w, int max_h)
{E_
    XSizeHints size_hints;
    long d;
    size_hints.min_width = min_w;
    size_hints.min_height = min_h;
    size_hints.max_width = max_w;
    size_hints.max_height = max_h;
    size_hints.width_inc = FONT_MEAN_WIDTH;
    size_hints.height_inc = FONT_PIX_PER_LINE;
    size_hints.base_width = min_w;
    size_hints.base_height = min_h;
    size_hints.flags = PBaseSize | PResizeInc | PMaxSize | PMinSize;

    if (wdt->options & WINDOW_USER_POSITION) {
	size_hints.x = wdt->x;
	size_hints.y = wdt->y;
	size_hints.flags |= USPosition | PPosition;
    }
    if (wdt->options & WINDOW_USER_SIZE) {
	size_hints.width = wdt->width;
	size_hints.height = wdt->height;
	size_hints.flags |= USSize | PSize;
    }
/* used as check if hints are not set when CMap is called */
    wdt->options |= WINDOW_SIZE_HINTS_SET;

    XSetWMNormalHints (CDisplay, wdt->winid, &size_hints);
    XSync (CDisplay, 0);
    XGetWMNormalHints (CDisplay, wdt->winid, &size_hints, &d);
    XSync (CDisplay, 0);
}

void CSetWindowResizable (const char *ident, int min_width, int min_height, int max_width, int max_height)
{E_
    Window w;
    int width, height;
    CWidget *wdt;

    wdt = CIdent (ident);
    w = wdt->winid;
    width = wdt->width;
    height = wdt->height;

    min_width = width - ((width - min_width) / FONT_MEAN_WIDTH) * FONT_MEAN_WIDTH;
    min_height = height - ((height - min_height) / FONT_PIX_PER_LINE) * FONT_PIX_PER_LINE;
    max_width = width - ((width - max_width) / FONT_MEAN_WIDTH) * FONT_MEAN_WIDTH;
    max_height = height - ((height - max_height) / FONT_PIX_PER_LINE) * FONT_PIX_PER_LINE;

    if (wdt->parentid == CRoot) {
	/* set the window manager to manage resizing */
	XWMHints wm_hints;
	XClassHint class_hints;

	class_hints.res_name = CAppName;
	class_hints.res_class = CAppName;
	wm_hints.flags = (InputHint | StateHint);
	wm_hints.input = True;
	wm_hints.initial_state = NormalState;

	XSetWMProperties (CDisplay, w, 0, 0, 0, 0, 0, &wm_hints, &class_hints);
	CSetWindowSizeHints (wdt, min_width, min_height, max_width, max_height);
    } else {
/* else, we must manage our own resizing. */
/* the member names do not mean what they are named here */
	XSelectInput (CDisplay, w, INPUT_MOTION | StructureNotifyMask);
	wdt->position |= WINDOW_RESIZABLE;
	wdt->mark1 = min_width;
	wdt->mark2 = min_height;
	wdt->firstcolumn = width;
	wdt->firstline = height;
/* we are not going to specify a maximum */
	wdt->numlines = FONT_PIX_PER_LINE;	/* resizing granularity x */
	wdt->resize_gran = FONT_MEAN_WIDTH;	/* resizing granularity x */
    }
}

extern char *init_geometry;

Window CDrawHeadedDialog (const char *identifier, Window parent, int x, int y, const char *label)
{E_
    Window win;
    CWidget *wdt;

    if ((parent == CRoot || !parent) && !override_redirect) {
	int width, height, bitmask;
	bitmask = 0;
	x = 0;
	y = 0;
	width = 10;
	height = 10;
	if (!CFirstWindow) {
	    if (init_geometry)
		bitmask = XParseGeometry (init_geometry, &x, &y, (unsigned int *) &width, (unsigned int *) &height);
	}
	win = (wdt = CSetupWidget (identifier, CRoot, x, y,
				   width, height, C_WINDOW_WIDGET,
				   INPUT_MOTION | StructureNotifyMask | FocusChangeMask | KeyPressMask | KeyReleaseMask,
				   COLOR_FLAT, 0))->winid;
	if (!CFirstWindow) {	/* create the GC the first time round and
				   CFirstWindow. when the window is closed,
				   the app gets a QuitApplication event */
	    CFirstWindow = win;

/* these options tell CSetWindowSizeHints() to give those hints to the WM: */
	    if (bitmask & (XValue | YValue))
		wdt->options |= WINDOW_USER_POSITION;
	    if (bitmask & (WidthValue | HeightValue))
		wdt->options |= WINDOW_USER_SIZE;
	}
	wdt->label = (char *) strdup (label);
	XSetIconName (CDisplay, win, wdt->label);
	XStoreName (CDisplay, win, wdt->label);
	{
	    Atom a[2];
	    a[0] = ATOM_WM_DELETE_WINDOW;
#if 0
	    a[1] = ATOM_WM_TAKE_FOCUS;
	    XChangeProperty (CDisplay, win, ATOM_WM_PROTOCOLS, XA_ATOM, 32,
		PropModeReplace, (unsigned char *) a, 2);
#else
	    XChangeProperty (CDisplay, win, ATOM_WM_PROTOCOLS, XA_ATOM, 32,
		PropModeReplace, (unsigned char *) a, 1);
#endif
#if 0
	    XSetWMProtocols (CDisplay, win, &ATOM_WM_DELETE_WINDOW, 1);
#endif
	}
	reset_hint_pos (WIDGET_SPACING + 2, WIDGET_SPACING + 2);
	wdt->position |= WINDOW_UNMOVEABLE;
	wdt->options |= WINDOW_NO_BORDER;
    } else {
	int w, h;
	CTextSize (&w, &h, label);
	win = CDrawDialog (identifier, parent, x, y);
	(CDrawText (catstrs (identifier, ".header", NULL), win, WIDGET_SPACING, WIDGET_SPACING + 2, label))->position |= POSITION_CENTRE;
	CGetHintPos (&x, &y);
#ifndef NEXT_LOOK
	(CDrawBar (win, WIDGET_SPACING, y, 10))->position |= POSITION_FILL;
	CGetHintPos (&x, &y);
#endif
	reset_hint_pos (WIDGET_SPACING + 2, y);
    }
    return win;
}

Window CDrawDialog (const char *identifier, Window parent, int x, int y)
{E_
    Window w;
    CWidget *wdt;
    w = (wdt = CSetupWidget (identifier, parent, x, y,
	     2, 2, C_WINDOW_WIDGET, INPUT_MOTION, COLOR_FLAT, 0))->winid;
    reset_hint_pos (WIDGET_SPACING + 2, WIDGET_SPACING + 2);
    return w;
}

/* returns the actual window that is a child  of the root window */
/* this is not the same as a main window, since the WM places each */
/* main window inside a cosmetic window */
Window CGetWMWindow (Window win)
{E_
    Window root, parent, *children;
    unsigned int nchildren;

    for (;;) {
	if (!XQueryTree (CDisplay, win, &root, &parent, &children, &nchildren))
	    break;
	if (children)
	    XFree ((char *) children);
	if (parent == CRoot)
	    return win;
	win = parent;
    }
    return 0;
}

/* I tested this procedure with kwm, fvwm95, fvwm, twm, olwm, without problems */
void CRaiseWMWindow (char *ident)
{E_
    Window wm_win;
    CWidget *w;
    XWindowChanges c;

    w = CIdent (ident);
    if (!w)
	return;
    wm_win = CGetWMWindow (w->mainid);
    if (!wm_win)
	return;
    c.stack_mode = Above;
    XConfigureWindow (CDisplay, wm_win, CWStackMode, &c);
    XFlush (CDisplay);
}

void CSetBackgroundPixmap (const char *ident, const char *data[], int w, int h, char start_char)
{E_
    XSetWindowAttributes xswa;
    CWidget *wdt;
    wdt = CIdent (ident);
    if (wdt->pixmap)
	XFreePixmap (CDisplay, wdt->pixmap);
    xswa.background_pixmap = wdt->pixmap = CCreatePixmap (data, w, h, start_char);
    if (xswa.background_pixmap)
	XChangeWindowAttributes (CDisplay, wdt->winid, CWBackPixmap, &xswa);
}

#define MAX_KEYS_IN_DIALOG 64

int find_letter_at_word_start (unsigned char *label, unsigned char *used_keys, int n)
{E_
    int c, j;
    for (j = 0; label[j]; j++) {	/* check for letters with an & in front of them */
	c = my_lower_case (label[j + 1]);
	if (!c)
	    break;
	if (label[j] == '&')
	    if (!memchr (used_keys, c, n))
		return label[j + 1];
    }
    c = my_lower_case (label[0]);
    if (c >= 'a' && c <= 'z')
	if (!memchr (used_keys, c, n))	/* check if first letter has not already been used */
	    return label[0];
    for (j = 1; label[j]; j++) {	/* check for letters at start of words that have not already been used */
	c = my_lower_case (label[j]);
	if (label[j - 1] == ' ' && c >= 'a' && c <= 'z')
	    if (!memchr (used_keys, c, n))
		return label[j];
    }
    for (j = 1; label[j]; j++) {	/* check for any letters that have not already been used */
	c = my_lower_case (label[j]);
	if (c >= 'a' && c <= 'z')
	    if (!memchr (used_keys, c, n))
		return label[j];
    }
    return 0;
}

int find_hotkey (CWidget * w)
{E_
    unsigned char used_keys[MAX_KEYS_IN_DIALOG + 3];
    const char *label;
    int n = 0;
    CWidget *p = w;
    label = w->label ? w->label : w->text.data;	/* text for text-widgets which don't have a label */
    if (!label)
	return 0;
    if (!*label)
	return 0;
    do {
	w = CNextFocus (w);
	if (!w || n == MAX_KEYS_IN_DIALOG)
	    return 0;
	if (w->hotkey < 256)
	    used_keys[n++] = my_lower_case (w->hotkey);
    } while ((unsigned long) w != (unsigned long) p);
    if (!n)
	return 0;
    return find_letter_at_word_start ((unsigned char *) label, used_keys, n);
}


CWidget *CDrawButton (const char *identifier, Window parent, int x, int y,
		      int width, int height, const char *label)
{E_
    CWidget *wdt;
    int w, h;
    CPushFont ("widget", 0);
    if (width == AUTO_WIDTH || height == AUTO_HEIGHT)
	CTextSize (&w, &h, label);
    if (width == AUTO_WIDTH)
	width = w + 4 + BUTTON_RELIEF * 2;
    if (height == AUTO_HEIGHT) {
	height = h + 4 + BUTTON_RELIEF * 2;
#ifdef NEXT_LOOK
	height++ ;
#endif	
    }	
    wdt = CSetupWidget (identifier, parent, x, y,
	    width, height, C_BUTTON_WIDGET, INPUT_BUTTON, COLOR_FLAT, 1);
    if (label)
	wdt->label = (char *) strdup (label);
    wdt->hotkey = find_hotkey (wdt);
    wdt->render = render_button;
    wdt->options |= WIDGET_TAKES_FOCUS_RING | WIDGET_HOTKEY_ACTIVATES;
    set_hint_pos (x + width + WIDGET_SPACING, y + height + WIDGET_SPACING);
    CPopFont ();
    return wdt;
}

CWidget *CDrawProgress (const char *identifier, Window parent, int x, int y,
			int width, int height, int p)
{E_
    CWidget *w;
    if ((w = CIdent (identifier))) {
	w->cursor = p;
	CSetWidgetPosition (identifier, x, y);
	CSetWidgetSize (identifier, width, height);
	CExpose (identifier);
    } else {
	w = CSetupWidget (identifier, parent, x, y,
	       width, height, C_PROGRESS_WIDGET, INPUT_EXPOSE, COLOR_FLAT, 0);
	w->cursor = p;
	set_hint_pos (x + width + WIDGET_SPACING, y + height + WIDGET_SPACING);
    }
    return w;
}

CWidget *CDrawBar (Window parent, int x, int y, int w)
{E_
    CWidget *wdt;
    wdt = CSetupWidget ("hbar", parent, x, y,
			 w, 3, C_BAR_WIDGET, INPUT_EXPOSE, COLOR_FLAT, 0);
    set_hint_pos (x + w + WIDGET_SPACING, y + 3 + WIDGET_SPACING);
    return wdt;
}

/* returns the text size. The result is one descent greater than the actual size */
void CTextSize (int *w, int *h, const char *str)
{E_
    char *p, *q = (char *) str;
    int w1, h1;
    if (!w)
	w = &w1;
    if (!h)
	h = &h1;
    *w = *h = 0;
    for (;;) {
	if (!(p = strchr (q, '\n')))
	    p = q + strlen (q);
	*h += FONT_PIX_PER_LINE;
	*w = max (CImageTextWidth (q, (unsigned long) p - (unsigned long) q), *w);
	if (!*p)
	    break;
	q = p + 1;
    }
}


CWidget *CDrawText (const char *identifier, Window parent, int x, int y, const char *fmt,...)
{E_
    va_list pa;
    char *str;
    int w, h;
    CWidget *wdt;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    CPushFont ("widget", 0);
    CTextSize (&w, &h, str);
    w += TEXT_RELIEF * 2 + 2;
    h += TEXT_RELIEF * 2 + 2;
    wdt = CSetupWidget (identifier, parent, x, y,
			w, h, C_TEXT_WIDGET, INPUT_EXPOSE, COLOR_FLAT, 0);
    wdt->text = CStr_dup (str);
    free (str);
    set_hint_pos (x + w + WIDGET_SPACING, y + h + WIDGET_SPACING);
    CPopFont ();
    return wdt;
}

CWidget *CDrawTextFixed (const char *identifier, Window parent, int x, int y, const char *fmt,...)
{E_
    va_list pa;
    char *str;
    int w, h;
    CWidget *wdt;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    CPushFont ("editor", 0);
    CTextSize (&w, &h, str);
    w += TEXT_RELIEF * 2 + 2;
    h += TEXT_RELIEF * 2 + 2;
    wdt = CSetupWidget (identifier, parent, x, y,
			w, h, C_TEXT_WIDGET, INPUT_EXPOSE, COLOR_FLAT, 0);
    wdt->options |= TEXT_FIXED;
    wdt->text = CStr_dup (str);
    free (str);
    set_hint_pos (x + w + WIDGET_SPACING, y + h + WIDGET_SPACING);
    CPopFont ();
    return wdt;
}

CWidget *CDrawStatus (const char *identifier, Window parent, int x, int y, int w, char *str)
{E_
    CWidget *wdt;
    int h;
    h = FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2;
    wdt = CSetupWidget (identifier, parent, x, y,
		     w, h, C_STATUS_WIDGET, INPUT_EXPOSE, COLOR_FLAT, 0);
    wdt->text = CStr_dup (str);
    set_hint_pos (x + w + WIDGET_SPACING, y + h + WIDGET_SPACING);
    return wdt;
}

void render_text (CWidget * w);

CWidget *CRedrawText (const char *identifier, const char *fmt,...)
{E_
    va_list pa;
    char *str;
    CWidget *wdt;
    int w, h;

    wdt = CIdent (identifier);
    if (!wdt)
	return 0;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    CPushFont ("widget", 0);
    CStr_free(&wdt->text);
    wdt->text = CStr_dup (str);

    CTextSize (&w, &h, str);
    w += TEXT_RELIEF * 2 + 2;
    h += TEXT_RELIEF * 2 + 2;

    CSetWidgetSize (identifier, w, h);
    render_text (wdt);
    free (str);
    CPopFont ();
    return wdt;
}

static Window get_wm_last_in_focus_stack (Window winid)
{
    Window r = 0;
    long *s = NULL;
    Atom type = 0;
    int format = 0;
    unsigned long nitems = 0, remaining = 0;
    Atom ATOM__NET_CLIENT_LIST_STACKING = 0;
    Atom ATOM_WINDOW = 0;

    ATOM__NET_CLIENT_LIST_STACKING = XInternAtom (CDisplay, "_NET_CLIENT_LIST_STACKING", False);
    ATOM_WINDOW = XInternAtom (CDisplay, "WINDOW", False);
    if (!ATOM__NET_CLIENT_LIST_STACKING)
        return 0;
    if (!XGetWindowProperty (CDisplay, CRoot, ATOM__NET_CLIENT_LIST_STACKING, 0 /* offset */ , 1024 /* length */ , False, AnyPropertyType,
                             &type, &format, &nitems, &remaining, (unsigned char **) &s) &&
        type == ATOM_WINDOW && s && format == 32 && nitems >= 2 && s[nitems - 2] != 0 && s[nitems - 1] == winid) {
        r = s[nitems - 2];
        XFree (s);
/* printf("last focused => 0x%lx\n", s[nitems - 2]); */
        return r;
    }
    XFree (s);
    return 0;
}

void focus_stack_remove_window (Window w);
void selection_clear (void);

/*
   Unmaps and destroys widget and frees memory.
   Only for a widget that has no children.
 */
int free_single_widget (int i_)
{E_
    Window wm_one_before_last = 0;
    const int i = i_;
    if (i && CIndex (i)) {
        CWidget *w = CIndex (i);
	if (w->winid) {
	    if (w->options & WIDGET_TAKES_SELECTION) {
		if (w->winid == XGetSelectionOwner (CDisplay, XA_PRIMARY))
		    XSetSelectionOwner (CDisplay, XA_PRIMARY, CFirstWindow, CurrentTime);
		if (w->winid == XGetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD))
		    XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, CFirstWindow, CurrentTime);
            }
	    if (CursorState.window == w->winid)
		set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	    XUnmapWindow (CDisplay, w->winid);
	    XDestroyWindow (CDisplay, w->winid);

/* We desire the behavior of reverting to the focus of the previous
window, say the application 'Merp'. If the user clicks on [X] in
cooledit's title bar, then cooledit closes immediately and the window
manager automatically sets the focus to Merp. If cooledit asks for
close-confirmation (say if an unsaved file) then focus remains with
cooledit until the user decides. Then when cooledit does close, the
window manager does NOT focus on Merp -- this is a usability bug. Only
CFirstWindow has this problem, since no other windows ask for
confirmation. To solve this problem we look at the window manager's
stack of open applications; we should be the last window in the stack
and the one before last is Merp: */
            if (CFirstWindow == w->winid && (wm_one_before_last = get_wm_last_in_focus_stack (w->winid))) {
                XSync (CDisplay, False);
                XSetInputFocus (CDisplay, wm_one_before_last, RevertToPointerRoot, CurrentTime);
                XSync (CDisplay, False);
            }
	    if (CFirstWindow == w->winid)
		CFirstWindow = 0;
	    focus_stack_remove_window (w->winid);	/* removes the window from the focus history stack */
	}
	if (w->label)
	    free (w->label);
	if (w->toolhint)
	    free (w->toolhint);
	if (w->headings)
	    free (w->headings);
	if (w->gl_graphicscontext) {
	    free (w->gl_graphicscontext);
	    w->gl_graphicscontext = 0;
	}
	if (w->ximage) {
	    if ((long) w->ximage->data == (long) w->graphic)
		w->graphic = NULL;
	    if (w->ximage->data) {
		free (w->ximage->data);
		w->ximage->data = 0;
	    }
	    XDestroyImage (w->ximage);
	}
	if (w->pixmap) {
	    XFreePixmap (CDisplay, w->pixmap);
	    w->pixmap = 0;
	}
	if (w->pixmap_mask) {
	    XFreePixmap (CDisplay, w->pixmap_mask);
	    w->pixmap_mask = 0;
	}
	if (w->graphic)
	    free (w->graphic);
	if (w->tab)
	    free (w->tab);
	if (w->destroy)
	    (*(w->destroy)) (CIndex (i));
	CStr_free (&(w->text));    /* for input history, this must come 
					   after the destroy, so that the text can be added to the input 
					   history by the destroy function before the text is free'd */
        if (w->textbox_funcs) {
            w->textbox_funcs->textbox_free_cb(w->textbox_funcs->hook1, w->textbox_funcs->hook2);
            free(w->textbox_funcs);
        }
	if (w->funcs)
	    free (w->funcs);
	if (w->free_user)
	    (*(w->free_user)) (w->user);
	else if (w->user && (w->options & WIDGET_FREE_USER_ON_DESTROY))
	    free (w->user);

	free (CIndex (i));
	CIndex (i) = NULL;
	while (!CIndex (last_widget - 1) && last_widget > 1)
	    last_widget--;
	return 1;
    } else
	return 0;
}

/*searches for the first widget in the list that has win as its parent
   and returns index */
int find_first_child_of (Window win)
{E_
    int i = 0;
    while (last_widget > i++)
	if (CIndex (i) != NULL)
	    if (CIndex (i)->parentid == win)
		return i;
    return 0;
}

int find_last_child_of (Window win)
{E_
    int i = last_widget;
    while (--i > 0)
	if (CIndex (i) != NULL)
	    if (CIndex (i)->parentid == win)
		return i;
    return 0;
}

/* int for_all_widgets (int (callback *) (CWidget *, void *, void *), void *data1, void *data2) */
long for_all_widgets (for_all_widgets_cb_t call_back, void *data1, void *data2)
{E_
    long (*callback) (CWidget *, void *, void *) = (for_all_widgets_cb_t) call_back;
    int i = last_widget;
    while (--i > 0)
	if (CIndex (i) != NULL)
	    if ((*callback) (CIndex (i), data1, data2))
		return 1;
    return 0;
}

int widget_of_window (Window win)
{E_
    int i = 0;
    while (last_widget > i++)
	if (CIndex (i) != NULL)
	    if (CIndex (i)->winid == win)
		return i;
    return 0;
}

int find_next_child_of (Window win, Window child)
{E_
    int i = widget_of_window (child);
    if (i)
	while (last_widget > i++)
	    if (CIndex (i) != NULL)
		if (CIndex (i)->parentid == win)
		    return i;
    return 0;
}

int find_previous_child_of (Window win, Window child)
{E_
    int i = widget_of_window (child);
    if (i)
	while (--i > 0)
	    if (CIndex (i) != NULL)
		if (CIndex (i)->parentid == win)
		    return i;
    return 0;
}

CWidget *CDialogOfWindow (Window window)
{E_
    for (;;) {
	CWidget *w;
	w = CWidgetOfWindow (window);
	if (!w)
	    break;
	if (w->kind == C_WINDOW_WIDGET)
	    return w;
	window = w->parentid;
    }
    return 0;
}

Window CFindParentMainWindow (Window parent)
{E_
    int i;
    if (parent == CRoot)
	return 0;
    if (!(i = widget_of_window (parent)))
	return 0;
    if (!CIndex (i)->mainid)
	return CIndex (i)->winid;
    return CIndex (i)->mainid;
}

/*recursively destroys a widget and all its descendants */
static void recursive_destroy_widgets (int i)
{E_
    int j;
    while ((j = find_first_child_of (CIndex (i)->winid)))
	recursive_destroy_widgets (j);
    free_single_widget (i);
}

void CFocusLast (void);

/*returns 1 on error --- not found. Destroys a widget by name and all its
   descendents */
int CDestroyWidget (const char *identifier)
{E_
    int i = find_ident (identifier);

    if (i) {
	recursive_destroy_widgets (i);
	CFocusLast ();
	return 0;
    } else
	return 1;
}


void CDestroyAll ()
{E_
    int j;
    while ((j = find_first_child_of (CRoot)))
	recursive_destroy_widgets (j);
}

void free_last_query_buttons (void);
void edit_replace_cmd (WEdit * edit, int again);
void free_selections (void);
void remove_all_watch (void);
void shutdown_xlocale(void);
void utf_tmp_buf_free(void);
void XChar2b_tmp_buf_free(void);
void wchar_t_tmp_buf_free(void);

void CShutdown (void)
{E_
    remove_all_watch ();
    CDestroyAll ();
#ifdef USE_XIM
/* set the XIM locale */
    shutdown_xlocale ();
#endif
    free (local_home_dir);
    local_home_dir = 0;
    free (temp_dir);
    temp_dir = 0;
    free_last_query_buttons ();
    edit_replace_cmd (0, 0);
    edit_search_cmd (0, 0);
    free_selections ();
    mouse_shut ();
    CFreeAllFonts ();
    utf_tmp_buf_free ();
    XChar2b_tmp_buf_free ();
    wchar_t_tmp_buf_free ();
    XCloseDisplay (CDisplay);
}

void drawstring_xy (Window win, int x, int y, const char *text)
{E_
    if (!text)
	return;
    if (!*text)
	return;
    CImageString (win, FONT_OFFSET_X + x, FONT_OFFSET_Y + y, text);
}


char *whereis_hotchar (const char *labl, int hotkey)
{E_
    unsigned char *label = (unsigned char *) labl;
    int i;
    if (hotkey <= ' ' || hotkey > 255)
	return 0;
    if (*label == hotkey)
	return (char *) label;
    for (i = 1; label[i]; i++)
	if (label[i - 1] == ' ' && label[i] == hotkey)
	    return (char *) label + i;
    return (char *) strchr ((char *) label, hotkey);
}

void underline_hotkey (Window win, int x, int y, const char *text, int hotkey)
{E_
    char *p;
    if (hotkey <= ' ' || hotkey > 255)
	return;
    if (!(p = whereis_hotchar (text, hotkey)))
	return;
    x += CImageTextWidth (text, (unsigned long) p - (unsigned long) text);
    y += FONT_BASE_LINE + FONT_PER_CHAR_DESCENT(hotkey) + 1;
    (*look->draw_hotkey_understroke) (win, x, y, hotkey);
}

void drawstring_xy_hotkey (Window win, int x, int y, const char *text, int hotkey)
{E_
    drawstring_xy (win, x, y, text);
    underline_hotkey (win, x, y, text, hotkey);
}

void render_button (CWidget * wdt)
{E_
    (*look->render_button) (wdt);
}

void render_bar (CWidget * wdt)
{E_
    (*look->render_bar) (wdt);
}

#define FB (TEXT_RELIEF + 1)
#define FS (TEXT_RELIEF + 1)

static int menu_width_calc (const unsigned char *p, const unsigned char *matching, int x, int *color, int *first_control, int *count)
{E_
    const unsigned char *q;
    q = matching ? matching : p;
    while (*q == *p && *q && *p) {
	if (*q >= ' ') {
	    const unsigned char *v = q;
	    while (*p == *q && *p >= ' ') {
		p++;
		q++;
		if (count)
		    (*count)++;
	    }
	    x += CImageTextWidth ((const char *) v, q - v);
	} else {
	    if (*q == '\034') {
		if (first_control)
		    *first_control = x;
	    } else if (*q == '\035') {
		if (first_control)
		    *first_control = x;
		x += FS;
	    } else if (color)
		*color = *q;
	    p++;
	    q++;
	    if (count)
		(*count)++;
	}
    }
    return x;
}

/* this is a zero flicker routine */
void render_status (CWidget * wdt, int expose)
{E_
    static Window lastwin = 0;
    static unsigned char lasttext[1024 + MAX_PATH_LEN] = "";
    Window win = CWindowOf (wdt);
    int last_width = 0;
    int h = CHeightOf (wdt);
    int w = CWidthOf (wdt);
    int l, x, x1 = 0, color = 0, n = 0;
    const unsigned char *p, *q;
    CPushFont ("widget", 0);
    x = TEXT_RELIEF + 1;	/* bevel is 1 */
    if (lastwin == win && !expose)
        x = menu_width_calc ((const unsigned char *) wdt->text.data, lasttext, x, &color, &x1, &n);
    q = (const unsigned char *) wdt->text.data + n;
    l = menu_width_calc (q, 0, x, 0, 0, 0);
    if (lastwin == win && !expose) {
        last_width = menu_width_calc (lasttext + n, 0, x, 0, 0, 0);
        if (l < last_width && l < w) {
	    CSetColor (COLOR_FLAT);
	    CRectangle (win, l, 0, min (w - l, last_width - l), h);
        }
    }
    CSetColor (color_palette (color % N_FAUX_COLORS));
    CSetBackgroundColor (COLOR_FLAT);
    for (p = q;; p++) {
	if (*p < ' ') {
	    CImageText (win, FONT_OFFSET_X + x, FONT_OFFSET_Y + TEXT_RELIEF + 1, (const char *) q, (unsigned long) p - (unsigned long) q);
	    x += CImageTextWidth ((const char *) q, (unsigned long) p - (unsigned long) q);
	    if (*p == '\035') {
#define RECT(x,y,w,h) XClearArea(CDisplay, win, x, y, w, h, 0)
		RECT (x, TEXT_RELIEF + 1, FS, FONT_PIX_PER_LINE);
		if (x - x1 + FB + FB - 2 > 0) {
		    render_bevel (win, x1 - FB, 0, x + FB - 1, h - 1, 1, 1);
		    RECT (x1 - FB + 1, 1, x - x1 + FB + FB - 2, TEXT_RELIEF + 1);
		    RECT (x1 - FB + 1, h - TEXT_RELIEF - 1, x - x1 + FB + FB - 2, TEXT_RELIEF);
		}
		x1 = x;
		x += FS;
	    } else if (*p == '\034') {
		if (x - x1 - FB - FB > 0) {
		    RECT (x1 + FB, 0, x - x1 - FB - FB, TEXT_RELIEF + 2);
		    RECT (x1 + FB, h - TEXT_RELIEF - 1, x - x1 - FB - FB, TEXT_RELIEF + 1);
		}
		x1 = x;
	    } else
		CSetColor (color_palette (*p % N_FAUX_COLORS));
	    if (!*p)
		break;
	    q = p + 1;
	}
    }
    lastwin = win;
    strncpy ((char *) lasttext, wdt->text.data, 1023);
    CPopFont ();
    return;
}

void render_text (CWidget * wdt)
{E_
    (*look->render_text) (wdt);
}

void render_window (CWidget * wdt)
{E_
    (*look->render_window) (wdt);
}

void render_progress (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    int p = wdt->cursor;

    Window win = wdt->winid;

    if (p > 65535)
	p = 65535;
    if (p < 0)
	p = 0;
    CSetColor (COLOR_FLAT);
    CRectangle (win, 4 + p * (w - 5) / 65535, 2, (65535 - p) * (w - 5) / 65535, h - 4);
    CSetColor (color_palette (3));
    CRectangle (win, 4, 4, p * (w - 9) / 65535, h - 8);
    render_bevel (win, 2, 2, 4 + p * (w - 9) / 65535, h - 3, 2, 0);
    render_bevel (win, 0, 0, w - 1, h - 1, 2, 1);
}

void render_sunken (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    render_bevel (win, 0, 0, w - 1, h - 1, 2, 1);
}

void render_bevel (Window win, int x1, int y1, int x2, int y2, int thick, int sunken)
{E_
    if (option_low_bandwidth)
	return;
    if (sunken & 1)
	(*look->render_sunken_bevel) (win, x1, y1, x2, y2, thick, sunken);
    else
	(*look->render_raised_bevel) (win, x1, y1, x2, y2, thick, sunken);
    CSetColor (COLOR_BLACK);
}

void expose_picture (CWidget * w);

void set_widget_position (CWidget * w, int x, int y)
{E_
    if (w->winid) {		/*some widgets have no window of there own */
	w->x = x;
	w->y = y;
	XMoveWindow (CDisplay, w->winid, x, y);
    } else {
#ifdef HAVE_PICTURE
	expose_picture (w);
	w->x = x;
	w->y = y;
	expose_picture (w);
#endif
    }
}

void CSetWidgetPosition (const char *ident, int x, int y)
{E_
    CWidget *w = CIdent (ident);
    if (!w)
	return;
    set_widget_position (w, x, y);
}

void configure_children (CWidget * wt, int w, int h)
{E_
    CWidget *wdt;
    int new_w, new_h, new_x, new_y, i;
    i = find_first_child_of (wt->winid);
    while (i) {
	wdt = CIndex (i);
	if (CGetFocus () == wdt->winid)		/* focus border must follow the widget */
	    destroy_focus_border ();
	if (wdt->resize) {
	    (*(wdt->resize)) (w, h, wt->width, wt->height, &new_w, &new_h, &new_x, &new_y);
	    if (wdt->height != new_h || wdt->width != new_w)
		CSetSize (wdt, new_w, new_h);
	    if (wdt->x != new_x || wdt->y != new_y)
		set_widget_position (wdt, new_x, new_y);
	} else {
	    if (wdt->position & POSITION_CENTRE)
		set_widget_position (wdt, (w - wdt->width) / 2, wdt->y);
	    if (wdt->position & POSITION_FILL)
		CSetSize (wdt, w - (WIDGET_SPACING + WINDOW_EXTRA_SPACING) - wdt->x, wdt->height);
	    if (wdt->position & POSITION_RIGHT)
		set_widget_position (wdt, wdt->x + w - wt->width, wdt->y);
	    if (wdt->position & POSITION_WIDTH)
		CSetSize (wdt, wdt->width + w - wt->width, wdt->height);
	    if (wdt->position & POSITION_BOTTOM)
		set_widget_position (wdt, wdt->x, wdt->y + h - wt->height);
	    if (wdt->position & POSITION_HEIGHT)
		CSetSize (wdt, wdt->width, wdt->height + h - wt->height);
	}
	if (CGetFocus () == wdt->winid)		/* focus border must follow the widget */
	    if ((wdt->options & WIDGET_TAKES_FOCUS_RING))
		create_focus_border (wdt, 2);
	i = find_next_child_of (wdt->parentid, wdt->winid);
    }
}

void CSetSize (CWidget * wt, int w, int h)
{E_
    int w_min, h_min;
    if (!wt)
	return;
    if (w == wt->width && h == wt->height)
	return;

    wt->resized = 1;

    if (w < 1)
	w = 1;
    if (h < 1)
	h = 1;

    if (wt->kind == C_WINDOW_WIDGET)
	configure_children (wt, w, h);
#if 0
    else if (wt->kind == C_RXVT_WIDGET)
	rxvt_resize_window (wt->rxvt, w, h);
#endif

/* redraw right and bottom borders */
    w_min = min (wt->width, w);
    h_min = min (wt->height, h);
    if (wt->kind == C_WINDOW_WIDGET)
	XClearArea (CDisplay, wt->winid, wt->width - 39, wt->height - 39, 39, 39, 1);
    XClearArea (CDisplay, wt->winid, w_min - 3, 0, 3, h_min, 1);
    XClearArea (CDisplay, wt->winid, 0, h_min - 3, w_min, 3, 1);
    wt->width = w;
    wt->height = h;
    if (wt->parentid == CRoot && wt->mapped)	/* afterstep doesn't like us to change the size of a mapped main window */
	return;
    XResizeWindow (CDisplay, wt->winid, w, h);
#ifdef USE_XIM
    set_status_position (wt);
#endif
}

void CSetWidgetSize (const char *ident, int w, int h)
{E_
    CWidget *wt = CIdent (ident);
    if (!wt)
	return;
    CSetSize (wt, w, h);
}

void CSetMovement (const char *ident, unsigned long position)
{E_
    CWidget *w;
    w = CIdent (ident);
    if (!w)
	return;
    w->position |= position;
}

void CCentre (char *ident)
{E_
    CSetMovement (ident, POSITION_CENTRE);
}

/* does a map as well */
void CSetSizeHintPos (const char *ident)
{E_
    int x, y;
    CWidget *w;
    get_hint_limits (&x, &y);
    w = CIdent (ident);
    x += WINDOW_EXTRA_SPACING;
    y += WINDOW_EXTRA_SPACING;
    if (!(w->options & WINDOW_NO_BORDER))
	y += (*look->get_window_resize_bar_thickness) ();
    XResizeWindow (CDisplay, w->winid, x, y);
    w->width = x;
    w->height = y;
    configure_children (w, x, y);
}

/* for mapping a main window. other widgets are mapped when created */
void CMapDialog (const char *ident)
{E_
    CWidget *w;
    w = CIdent (ident);
    if (!w)
	return;
    if (w->kind != C_WINDOW_WIDGET)
	return;
    if (w->parentid == CRoot
	&& !(w->options & WINDOW_SIZE_HINTS_SET)) {
/* A main window with WM size hints not configured. */
	CSetWindowSizeHints (w, w->width, w->height, w->width, w->height);
    }
    XMapWindow (CDisplay, w->winid);	/* shows the window */
    XFlush (CDisplay);
}
