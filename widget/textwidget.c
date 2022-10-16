/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* textwidget.c - for drawing a scrollable text window widget
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
#include "edit.h"
#include "editcmddef.h"

#include "coollocal.h"
#include "mousemark.h"


extern struct look *look;

int option_text_fg_normal = 0;
int option_text_fg_bold = 1;
int option_text_fg_italic = 18;

int option_text_bg_normal = 26;
int option_text_bg_marked = 25;
int option_text_bg_highlighted = 12;

static long current;

extern int calc_text_pos2 (CWidget * w, long b, long *q, int l);
extern void convert_text2 (CWidget * w, long bol, long from, cache_type *line, cache_type *eol, int x, int x_max, int row, struct _book_mark **bookmarks_not_used, int n_bookmarks_not_used);
void edit_translate_xy (int xs, int ys, int *x, int *y);

static int text_ypixel_to_row (int y)
{E_
    int row;
    row = y / FONT_PIX_PER_LINE + 1;
    return row;
}

/* returns the position in the edit buffer of a window click */
long text_get_click_pos (CWidget * w, int x, int y)
{E_
    CStr s;
    long click, c, q;
    int width;
    width = w->options & TEXTBOX_WRAP ? (w->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000;
    s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
    if (y > 1)
	c = strmovelines (s.data, w->current, y - 1, width);
    else
	c = w->current;
    if (y > 0)
	click = strmovelines (s.data, c, 1, width);
    else
	click = w->current;
    if (w->options & TEXTBOX_MARK_WHOLE_LINES) {
	if (click == c && y > 0) {
	    calc_text_pos2 (w, click, &q, 32000);	/* this is to get the last line */
	    return q;
	}
	return click;
    } else
	calc_text_pos2 (w, click, &q, x);
    return q;
}

static void xy (int x, int y, int *x_return, int *y_return)
{E_
    edit_translate_xy (x, y, x_return, y_return);
}

static long cp (CWidget * w, int x, int y)
{E_
    int row;
    row = text_ypixel_to_row (y);
    return text_get_click_pos (w, --x, --row);
}

/* return 1 if not marked */
static int marks (CWidget * w, long *start, long *end)
{E_
    if (w->mark1 ==  w->mark2)
	return 1;
    *start = min (w->mark1, w->mark2);
    *end = max (w->mark1, w->mark2);
    return 0;
}

int range (CWidget * w, long start, long end, int click)
{E_
    return (start <= click && click < end);
}

static void move_mark (CWidget * w)
{E_
    w->mark2 = w->mark1 = current;
}

static void fin_mark (CWidget * w)
{E_
    w->mark2 = w->mark1 = -1;
}

static void release_mark (CWidget * w, XEvent * event)
{E_
    w->mark2 = current;
    if (w->mark2 != w->mark1 && event) {
	selection_clear ();
	XSetSelectionOwner (CDisplay, XA_PRIMARY, w->winid, event->xbutton.time);
	XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, w->winid, event->xbutton.time);
    }
}

static char *get_block (CWidget * w, long start_mark, long end_mark, int *type, int *l)
{E_
    CStr s;
    char *t, *t2;
    *l = labs (w->mark2 - w->mark1);
    t = CMalloc (*l + 1);
    s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
    memcpy (t, s.data + min (w->mark1, w->mark2), *l);
    t[*l] = 0;

    t2 = str_strip_nroff ((char *) t, l);

    free (t);
    t2[*l] = 0;
    if (w->options & TEXTBOX_FILE_LIST) {
#ifdef HAVE_DND
	char *p;
	int i;
	p = CDndFileList (t2, l, &i);
	if (!p) {
	    free (t2);
	    return 0;
	}
	free (t2);
	t2 = p;
	if (i == 1)
	    *type = DndFile;
	else
#endif
	    *type = DndFiles;
    } else {
	*type = DndText;
    }

    return t2;
}

static void move (CWidget * w, long click, int y)
{E_
    int h, row;
    row = text_ypixel_to_row (y);
    current = click;
    if (w->mark2 == -1)
	w->mark1 = current;
    h = (w->height - TEXTBOX_BDR) / FONT_PIX_PER_LINE;
    if (row > h && w->firstline < w->numlines - h - 2)
	CSetTextboxPos (w, TEXT_SET_LINE, w->firstline + row - h);
    if (row < 1)
	CSetTextboxPos (w, TEXT_SET_LINE, w->firstline + row - 1);
    w->mark2 = click;
}

static void motion (CWidget * w, long click)
{E_
    w->mark2 = click;
}

struct mouse_funcs textbox_mouse_mark = {
    0,
    (void (*)(int, int, int *, int *)) xy,
    (long (*)(void *, int, int)) cp,
    (int (*)(void *, long *, long *)) marks,
    (int (*)(void *, long, long, long)) range,
    (void (*)(void *)) fin_mark,
    (void (*)(void *)) move_mark,
    (void (*)(void *, XEvent *)) release_mark,
    (char *(*)(void *, long, long, int *, int *)) get_block,
    (void (*)(void *, long, int)) move,
    (void (*)(void *, long)) motion,
    0,
    0,
    0,
    0,
    DndText
};


static CStr CDrawTextbox_basic_text_cb (void *hook1, void *hook2)
{E_
    CStr r;
    r.data = (char *) hook1;
    r.len = strlen(r.data);
    return r;
}

static void CDrawTextbox_basic_free_cb (void *hook1, void *hook2)
{E_
    (void) hook2;
    free(hook1);
}

CWidget *CDrawTextbox (const char *identifier, Window parent, int x, int y, int width, int height, int line,
		       int column, const char *text, long options)
{E_
    return CDrawTextboxManaged (identifier, parent, x, y, width, height, line, column,
				CDrawTextbox_basic_text_cb, CDrawTextbox_basic_free_cb,
				(char *) strdup (text), 0, options);
}

CWidget *CDrawTextboxManaged (const char *identifier, Window parent, int x, int y,
		       int width, int height, int line, int column, 
                       CStr (*get_cb) (void *, void *),
                       void (*free_cb) (void *, void *),
                       void *hook1, void *hook2, int options)
{E_
    char *scroll;
    int numlines;
    CWidget *wdt;
    CStr s;

    int w, h;

    s = (*get_cb) (hook1, hook2);

    CPushFont ("editor", 0);
    if (width == AUTO_WIDTH || height == AUTO_HEIGHT)
	CTextSize (&w, &h, s.data);
    if (width == AUTO_WIDTH)
	width = w + 6;
    if (height == AUTO_HEIGHT)
	height = h + 6;

    wdt = CSetupWidget (identifier, parent, x, y,
			width, height, C_TEXTBOX_WIDGET, INPUT_KEY,
			color_palette (option_text_bg_normal), 1);
    wdt->funcs = mouse_funcs_new (wdt, &textbox_mouse_mark);

    wdt->textbox_funcs = CMalloc(sizeof(*wdt->textbox_funcs));
    wdt->textbox_funcs->textbox_text_cb = get_cb;
    wdt->textbox_funcs->textbox_free_cb = free_cb;
    wdt->textbox_funcs->hook1 = hook1;
    wdt->textbox_funcs->hook2 = hook2;

    xdnd_set_type_list (CDndClass, wdt->winid, xdnd_typelist_send[DndText]);

    wdt->options = options | WIDGET_TAKES_SELECTION;
    numlines = strcountlines (s.data, 0, 1000000000, options & TEXTBOX_WRAP ? (wdt->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000) + 1;

    wdt->firstline = 0;
    wdt->firstcolumn = 0;
    wdt->cursor = 0;
    wdt->current = 0;
    wdt->numlines = numlines;

    CSetTextboxPos (wdt, TEXT_SET_LINE, line);
    CSetTextboxPos (wdt, TEXT_SET_COLUMN, column);

    if (height > 80) {
/* this will also set the hint position, set_hint_pos() */
	wdt->vert_scrollbar = CDrawVerticalScrollbar (scroll = catstrs (identifier, ".vsc", NULL), parent,
		x + width + WIDGET_SPACING, y, height, AUTO_WIDTH, 0, 0);
	CSetScrollbarCallback (wdt->vert_scrollbar->ident, wdt->ident, link_scrollbar_to_textbox);
    } else {
	set_hint_pos (x + width + WIDGET_SPACING, y + height + WIDGET_SPACING);
    }
    CPopFont ();
    return wdt;
}


int CSetTextboxPos (CWidget * w, int which, long p);

CWidget *CRedrawTextbox (const char *identifier, const char *text, int preserve)
{E_
    CWidget *w = CIdent (identifier);
    if (!w)
	return 0;
    if (w->textbox_funcs->textbox_text_cb != CDrawTextbox_basic_text_cb)
	abort ();
    return CRedrawTextboxManaged (identifier, CDrawTextbox_basic_text_cb, CDrawTextbox_basic_free_cb,
				  (char *) strdup (text), 0, preserve);
}

/* redraws the text box. If preserve is 0 then view position is reset to 0 */
CWidget *CRedrawTextboxManaged (const char *identifier,
                        CStr (*get_cb) (void *, void *),
                        void (*free_cb) (void *, void *),
                        void *hook1, void *hook2,
                        int preserve)
{E_
    CStr s;
    CWidget *w = CIdent (identifier);
    int numlines, firstline, firstcolumn, cursor;

    if (!w)
	return 0;

    (*w->textbox_funcs->textbox_free_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
    w->textbox_funcs->textbox_text_cb = get_cb;
    w->textbox_funcs->textbox_free_cb = free_cb;
    w->textbox_funcs->hook1 = hook1;
    w->textbox_funcs->hook2 = hook2;
    s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);

    CPushFont ("editor", 0);
    numlines = strcountlines (s.data, 0, 1000000000, w->options & TEXTBOX_WRAP ? (w->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000) + 1;
    w->numlines = numlines;
    firstline = w->firstline;
    firstcolumn = w->firstcolumn;
    cursor = w->cursor;

    w->firstline = 0;
    w->current = 0;
    w->firstcolumn = 0;
    w->cursor = 0;
    w->mark1 = w->mark2 = -1;

    if (preserve) {
	CSetTextboxPos (w, TEXT_SET_LINE, firstline);
	CSetTextboxPos (w, TEXT_SET_COLUMN, firstcolumn);
	CSetTextboxPos (w, TEXT_SET_CURSOR_LINE, cursor);
    }
    CExpose (identifier);
    CPopFont ();

    return w;
}

CStr CGetTextBoxText (CWidget * w)
{E_
    return (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
}

/* result must not be free'd, and must be used immediately */
char *CGetTextBoxLine (CWidget * w, int i)
{E_
    CStr s;
    int width;
    char *r;
    CPushFont ("editor", 0);
    width = w->options & TEXTBOX_WRAP ? (w->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000;
    s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
    r = strline (s.data, strmovelines (s.data, w->current, i - w->firstline, width));
    CPopFont ();
    return r;
}

/* clears the text box */
CWidget *CClearTextbox (const char *identifier)
{E_
    CWidget *w;
    w = CIdent (identifier);
    if (w) {
        if (w->textbox_funcs->textbox_text_cb != CDrawTextbox_basic_text_cb)
            abort();
	free (w->textbox_funcs->hook1);
        w->textbox_funcs->hook1 = (char *) strdup("");
	w->numlines = 0;
	w->firstline = w->firstcolumn = 0;
	w->mark1 = w->mark2 = 0;
	CExpose (identifier);
    }
    return w;
}

CWidget *CDrawManPage (const char *identifier, Window parent, int x, int y,
	   int width, int height, int line, int column, const char *text)
{E_
    CWidget *w;
    w = CDrawTextbox (identifier, parent, x, y, width, height, line, column, text, TEXTBOX_MAN_PAGE);
    return w;
}



/*
   If which is TEXT_SET_POS the current offset of the top right
   corner is set to p.
   returns non-zero if anything actually changed.
 */
int CSetTextboxPos (CWidget * wdt, int which, long p)
{E_
    long q;
    int width, i, j;
    if (p < 0)
	p = 0;
    CPushFont ("editor", 0);

    width = wdt->options & TEXTBOX_WRAP ? (wdt->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000;

    switch (which) {
    case TEXT_SET_COLUMN:
	i = wdt->firstcolumn;
	wdt->firstcolumn = p;
	CPopFont ();
	return (i != wdt->firstcolumn);
    case TEXT_SET_LINE:
	i = wdt->firstline;
	if (p >= wdt->numlines)
	    p = wdt->numlines - 1;
	if (p < 0)
	    p = 0;
	if (wdt->kind == C_FIELDED_TEXTBOX_WIDGET) {
	    wdt->firstline = p;
	} else {
            CStr s;
            s = (*wdt->textbox_funcs->textbox_text_cb) (wdt->textbox_funcs->hook1, wdt->textbox_funcs->hook2);
	    q = strmovelines (s.data, wdt->current, p - wdt->firstline, width);
	    wdt->firstline += strcountlines (s.data, wdt->current, q - wdt->current, width);
	    wdt->current = q;
	}
	CPopFont ();
	return (i != wdt->firstline);
    case TEXT_SET_POS:
	i = wdt->firstline;
	if (wdt->kind == C_FIELDED_TEXTBOX_WIDGET) {
	    break;
	} else {
            CStr s;
            s = (*wdt->textbox_funcs->textbox_text_cb) (wdt->textbox_funcs->hook1, wdt->textbox_funcs->hook2);
	    wdt->firstline += strcountlines (s.data, wdt->current, p - wdt->current, width);
	    wdt->current = p;
	}
	CPopFont ();
	return (i != wdt->firstline);
    case TEXT_SET_CURSOR_LINE:
	i = wdt->firstline;
	j = wdt->cursor;
	if (p < 0)
	    p = 0;
	if (p >= wdt->numlines)
	    p = wdt->numlines - 1;
	wdt->cursor = p;
	if (p < wdt->firstline)
	    CSetTextboxPos (wdt, TEXT_SET_LINE, p);
	else if (p > wdt->firstline + (wdt->height - FONT_PIX_PER_LINE - 6) / FONT_PIX_PER_LINE)
	    CSetTextboxPos (wdt, TEXT_SET_LINE, p - (wdt->height - FONT_PIX_PER_LINE - 6) / FONT_PIX_PER_LINE);
	CPopFont ();
	return (i != wdt->firstline || j != wdt->cursor);
    }
/* NLS ? */
    CError ("settextpos: command not found.\n");
    CPopFont ();
    return 0;
}


static void text_print_line (CWidget * w, long b, int row)
{E_
    edit_draw_proportional (w,
                     (converttext_cb_t) convert_text2,
		     (calctextpos_cb_t) calc_text_pos2,
			    -w->firstcolumn * FONT_MEAN_WIDTH, w->winid,
			    w->width, b, row, row * FONT_PIX_PER_LINE + EDIT_TEXT_VERTICAL_OFFSET, 0,
			    FONT_PER_CHAR(' ') * TAB_SIZE, 0, 0);
}



/*
   ->firstline   is line number of the top line in the window.
   ->firstcolumn is column shift (positive).
   ->current     is actual char position of first line in display.
   ->numlines    is the total number of lines.
   ->cursor      is the number of the highlighted line.
   First three must be initialised to proper values (e.g. 0, 0 and 0).
 */

extern int EditExposeRedraw;
extern int EditClear;
extern int highlight_this_line;
extern unsigned long edit_normal_background_color;

long render_textbox (CWidget * w, int redrawall, int event_type)
{E_
    CStr s;
    long b;
    int c = 0, r = 0, row, height, isfocussed, wrap_width = 32000,
     curs, lines_drawn = 0;
    int pending_events = 0;
    unsigned long event_mask = 0;

    s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);

    CPushFont ("editor", 0);
    if (w->options & TEXTBOX_WRAP) {
	wrap_width = (w->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH;
	if (w->resized) {	/* a resize will change the number lines if text is wrapping */
	    int firstline = w->firstline;
	    w->numlines = strcountlines (s.data, 0, 1000000000, wrap_width) + 1;
	    w->firstline = 0;
	    w->current = w->cursor = 0;
	    CSetTextboxPos (w, TEXT_SET_LINE, firstline);
	    w->resized = 0;	/* done */
	}
    }
    if (redrawall) {
	EditExposeRedraw = 1;
	EditClear = 1;
    }
    b = w->current;
    height = w->height / FONT_PIX_PER_LINE + 1;

    isfocussed = (w->winid == CGetFocus ());
    curs = !(w->options & TEXTBOX_NO_CURSOR || w->mark1 != w->mark2);	/* don't draw the cursor line */

    edit_set_foreground_colors (color_palette (option_text_fg_normal), color_palette (option_text_fg_bold), color_palette (option_text_fg_italic));
    edit_set_background_colors (color_palette (option_text_bg_normal), color_palette (0), color_palette (option_text_bg_marked), color_palette (9), color_palette (option_text_bg_highlighted));

    for (row = 0; row < height; row++) {
	if (row + w->firstline == w->cursor && isfocussed && curs)
	    highlight_this_line = 1;
	else
	    highlight_this_line = 0;
	if (row + w->firstline < w->numlines) {
	    c = strmovelines (s.data, b, 1, wrap_width);
	    if (c != b) {	/* at last line strmovelines cannot move */
		r = s.data[c];
		s.data[c] = 0;	/* mark where line wraps */
	    }
	    lines_drawn++;
            if (!pending_events)
	        text_print_line (w, b, row);
	    if (c != b)
		s.data[c] = r;	/* remove mark */
	    b = c;
	} else {
            if (!pending_events)
	        text_print_line (w, s.len, row);	/* print blank lines */
	}
        /* check if there more events coming of the SAME type of event that generated this render */
        if (!pending_events) {
            if (event_type == ButtonPress)
                event_mask |= ButtonPressMask;
            if (event_type == ButtonRelease)
                event_mask |= ButtonReleaseMask;
            if (event_type == MotionNotify)
                event_mask |= ButtonMotionMask;
            if (event_type == KeyPress)
                event_mask |= KeyPressMask;
            if (event_mask && CCheckWindowEvent (w->winid, event_mask, 0))
                pending_events = 1;
        }
    }

    EditExposeRedraw = 0;
    EditClear = 0;
    (*look->render_textbox_tidbits) (w, isfocussed);
    CSetColor (edit_normal_background_color);
    CLine (w->winid, 3, 3, 3, w->height - 4);
    CPopFont ();
    return lines_drawn;
}

/*
   Count the number of lines that would be printed
   by the above routine, but don't print anything.
   If all is non-zero then count all the lines.
 */
long count_textbox_lines (CWidget * wdt, int all)
{E_
    CStr s;
    int nroff, col = 0, row = 0, height, width;
    long from;
    unsigned char c;
    int wrap_mode;
    const unsigned char *text;

    s = (*wdt->textbox_funcs->textbox_text_cb) (wdt->textbox_funcs->hook1, wdt->textbox_funcs->hook2);

    CPushFont ("editor", 0);
    nroff = (wdt->options & TEXTBOX_MAN_PAGE);
    wrap_mode = (wdt->options & TEXTBOX_WRAP);
    if (nroff)
	wrap_mode = 0;

    text = (unsigned char *) s.data;
    height = wdt->height / FONT_PIX_PER_LINE;
    width = (wdt->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH;
    if (all)
	from = 0;
    else
	from = wdt->current;

    for (; row < height || all; from++) {
	c = text[from];
	if (!c)
	    break;
	if ((c == '\n') || (col == width && wrap_mode)) {
	    col = 0;
	    row++;
	    if (c == '\n' || row >= height)
		continue;
	}
	if (c == '\r')
	    continue;
	if (c == '\t') {
	    col = (col / 8) * 8 + 8;
	    continue;
	}
	col++;
    }
    CPopFont ();
    return row + 1;
}

/* move the text box cursor or the text window if there isn't one */
int CTextboxCursorMove (CWidget * wdt, KeySym key)
{E_
    int handled = 0;
    CPushFont ("editor", 0);
/* when text is highlighted, the cursor must be off */
    if (wdt->options & TEXTBOX_NO_CURSOR || wdt->mark1 != wdt->mark2) {
	int to_move = 0;
	switch ((int) key) {
	case CK_Up:
	    handled = 1;
	    to_move = -1;
	    break;
	case CK_Down:
	    handled = 1;
	    to_move = 1;
	    break;
	case CK_Page_Up:
	    handled = 1;
	    to_move = 1 - wdt->height / FONT_PIX_PER_LINE;
	    break;
	case CK_Page_Down:
	    handled = 1;
	    to_move = wdt->height / FONT_PIX_PER_LINE - 1;
	    break;
	case CK_Home:
	    handled = 1;
	    to_move = -32000;
	    break;
	case CK_End:
	    handled = 1;
	    to_move = 32000;
	    break;
	case CK_Left:
	    handled = 1;
	    if (wdt->firstcolumn > 0)
		wdt->firstcolumn--;
	    break;
	case CK_Right:
	    handled = 1;
	    wdt->firstcolumn++;
	    break;
	}
        if (handled)
	    CSetTextboxPos (wdt, TEXT_SET_LINE, wdt->firstline + to_move);
    } else {
	switch ((int) key) {
	case CK_Up:
	    handled = 1;
	    wdt->cursor--;
	    break;
	case CK_Down:
	    handled = 1;
	    wdt->cursor++;
	    break;
	case CK_Page_Up:
	    handled = 1;
	    wdt->cursor -= (wdt->height / FONT_PIX_PER_LINE - 1);
	    break;
	case CK_Page_Down:
	    handled = 1;
	    wdt->cursor += (wdt->height / FONT_PIX_PER_LINE - 1);
	    break;
	case CK_Home:
	    handled = 1;
	    wdt->cursor = 0;
	    break;
	case CK_End:
	    handled = 1;
	    wdt->cursor = wdt->numlines;
	    break;
	case CK_Left:
	    handled = 1;
	    if (wdt->firstcolumn > 0) {
		wdt->firstcolumn--;
	    }
	    break;
	case CK_Right:
	    handled = 1;
	    wdt->firstcolumn++;
	    break;
	}
        if (handled)
	    CSetTextboxPos (wdt, TEXT_SET_CURSOR_LINE, wdt->cursor);	/* just does some checks */
    }
    CPopFont ();
    return handled;
}

static void text_mouse_mark (CWidget * w, XEvent * event, CEvent * ce)
{E_
    CPushFont ("editor", 0);
    mouse_mark (event, ce->double_click, w->funcs);
    CPopFont ();
}

/* gets selected text into selection structure, stripping nroff */
static void text_get_selection (CWidget * w)
{E_
    CStr s, r;
    char *t;
    int len;
    len = labs (w->mark2 - w->mark1);
    s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
    t = CMalloc (len + 1);
    memcpy (t, s.data + min (w->mark1, w->mark2), len);
    t[len] = 0;
    r.data = (char *) str_strip_nroff ((char *) t, &r.len);
    if (!r.data)
        r = CStr_dup("");
    r.data[r.len] = 0;
    selection_replace(r);
    CStr_free(&r);
    free (t);
}

void selection_send (XSelectionRequestEvent * rq);

int eh_textbox (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    int handled = 0, redrawall, count;

    redrawall = 0;
    switch (xevent->type) {
    case SelectionRequest:
	text_get_selection (w);
	selection_send (&(xevent->xselectionrequest));
	return 1;
    case Expose:
	if (!xevent->xexpose.count)
	    redrawall = 1;
	break;
    case ClientMessage:
	w->mark1 = w->mark2 = 0;
	break;
    case ButtonPress:
	CFocus (w);
	CPushFont ("editor", 0);
	if (xevent->xbutton.button == Button1)
	    w->cursor = (xevent->xbutton.y - TEXTBOX_BDR) / FONT_PIX_PER_LINE + w->firstline;
	if (w->cursor > w->numlines - 1)
	    w->cursor = w->numlines - 1;
	if (w->cursor < 0)
	    w->cursor = 0;
	cwevent->ident = w->ident;
	cwevent->xt = (xevent->xbutton.x - 7) / FONT_MEAN_WIDTH + w->firstcolumn;
	cwevent->yt = w->cursor;
	CPopFont ();
    case ButtonRelease:
    case MotionNotify:
	if (!xevent->xmotion.state && xevent->type == MotionNotify)
	    return 0;
	resolve_button (xevent, cwevent);
	if ((cwevent->button == Button4 || cwevent->button == Button5)
	    && (xevent->type == ButtonRelease)) {
	    /* ahaack: wheel mouse mapped as button 4 and 5 */
	    CPushFont ("editor", 0);
            if ((xevent->xbutton.state & ShiftMask)) {
                if (cwevent->button == Button5)
	            CSetTextboxPos (w, TEXT_SET_LINE, w->firstline + 1);
                else
	            CSetTextboxPos (w, TEXT_SET_LINE, w->firstline - 1);
            } else {
                int delta;
                delta = (w->height / 6 / FONT_PIX_PER_LINE - 1);
                if (delta < 2)
                    delta = 2;
                if (cwevent->button == Button5)
	            CSetTextboxPos (w, TEXT_SET_LINE, w->firstline + delta);
                else
	            CSetTextboxPos (w, TEXT_SET_LINE, w->firstline - delta);
            }
	    CPopFont ();
            handled = 1;
	    break;
	}
	text_mouse_mark (w, xevent, cwevent);
	break;
    case FocusIn:
    case FocusOut:
	break;
    case KeyPress:
	cwevent->ident = w->ident;
	if (!(TEXTBOX_NO_KEYS & w->options))
	    handled = CTextboxCursorMove (w, cwevent->command);
	break;
    default:
	return 0;
    }

/* Now draw the changed text box, count will contain
   the number of textlines displayed */
    count = render_textbox (w, redrawall, xevent->type);

/* now update the scrollbar position */
    if (w->vert_scrollbar) {
	w->vert_scrollbar->firstline = (double) 65535.0 *w->firstline / w->numlines;
	w->vert_scrollbar->numlines = (double) 65535.0 *count / w->numlines;
	w->vert_scrollbar->options = 0;
	render_scrollbar (w->vert_scrollbar);
    }

    return handled;
}

void link_scrollbar_to_textbox (CWidget * scrollbar, CWidget * textbox, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{E_
    int redrawtext = 0, count = -1, c;
    static int r = 0;
    CPushFont ("editor", 0);
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, (double) scrollbar->firstline * textbox->numlines / 65535.0);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline - (textbox->height / FONT_PIX_PER_LINE - 2));
	    break;
	case 2:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline - 1);
	    break;
	case 5:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline + 1);
	    break;
	case 4:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline + (textbox->height / FONT_PIX_PER_LINE - 2));
	    break;
	}
    }
    if (xevent->type == ButtonRelease)
	count = render_textbox (textbox, 0, ButtonRelease);
    else {
	c = CCheckWindowEvent (0, ButtonReleaseMask | ButtonMotionMask, 1);
	if (redrawtext) {
	    if (!c) {
		render_textbox (textbox, 0, 0);
		r = 0;
	    } else {
		r = 1;
	    }
	} else if (c && r) {
	    render_textbox (textbox, 0, 0);
	    r = 0;
	}
    }
    if (count < 0)
	count = count_textbox_lines (textbox, 0);
    if (!count)
	count = 1;
    scrollbar->firstline = (double) 65535.0 *textbox->firstline / textbox->numlines;
    scrollbar->numlines = (double) 65535.0 *count / textbox->numlines;
    CPopFont ();
    return;
}




